// Copyright 2017 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <hwreg/mmio.h>

#include <magenta/assert.h>
#include <mxtl/type_support.h>
#include <stdint.h>

// This file provides some helpers for accessing bitfields in registers.
//
// Example usage:
//
//   // Define bitfields for an "AuxControl" register.
//   class AuxControl : public RegisterBase<uint32_t> {
//   public:
//       // Define a single-bit field.
//       DEF_BIT(31, enabled);
//       // Define a 5-bit field, from bits 20-24 (inclusive).
//       DEF_FIELD(24, 20, message_size);
//
//       // Returns an object representing the register's type and address.
//       static auto Get() { return RegisterAddr<AuxControl>(0x64010); }
//   };
//
//   void Example1(RegisterIo* reg_io)
//   {
//       // Read the register's value from MMIO.  "reg" is a snapshot of the
//       // register's value which also knows the register's address.
//       auto reg = AuxControl::Get().ReadFrom(reg_io);
//
//       // Read this register's "message_size" field.
//       uint32_t size = reg.message_size();
//
//       // Change this field's value.  This modifies the snapshot.
//       reg.set_message_size(1234);
//
//       // Write the modified register value to MMIO.
//       reg.WriteTo(reg_io);
//   }
//
//   // It is also possible to write a register without having to read it
//   // first:
//   void Example2(RegisterIo* reg_io)
//   {
//       // Start off with a value that is initialized to zero.
//       auto reg = AuxControl::Get().FromValue(0);
//       // Fill out fields.
//       reg.set_message_size(2345);
//       // Write the register value to MMIO.
//       reg.WriteTo(reg_io);
//   }
//
// Note that this produces efficient code with GCC and Clang, which are
// capable of optimizing away the intermediate objects.
//
// The arguments to DEF_FIELD() are organized to match up with Intel's
// documentation.  For example, if the docs specify a field as:
//   23:0  Data M value
// then that translates to:
//   DEF_FIELD(23, 0, data_m_value)
// To match up, we put the upper bit first and use an inclusive bit range.

namespace hwreg {

// An instance of RegisterBase represents a staging copy of a register,
// which can be written to the register itself.  It knows the register's
// address and stores a value for the register.
//
// Normal usage is to create classes that derive from RegisterBase and
// provide methods for accessing bitfields of the register.  RegisterBase
// does not provide a constructor because constructors are not inherited by
// derived classes by default, and we don't want the derived classes to
// have to declare constructors.
template <class IntType> class RegisterBase {
public:
    using ValueType = IntType;
    uint32_t reg_addr() { return reg_addr_; }
    void set_reg_addr(uint32_t addr) { reg_addr_ = addr; }

    ValueType reg_value() { return reg_value_; }
    ValueType* reg_value_ptr() { return &reg_value_; }
    void set_reg_value(IntType value) { reg_value_ = value; }

    void ReadFrom(RegisterIo* reg_io) { reg_value_ = reg_io->Read<ValueType>(reg_addr_); }
    void WriteTo(RegisterIo* reg_io) { reg_io->Write(reg_addr_, reg_value_ & ~rsvdz_mask_); }

protected:
    ValueType rsvdz_mask_ = 0;
private:
    uint32_t reg_addr_ = 0;
    ValueType reg_value_ = 0;
};

// An instance of RegisterAddr represents a typed register address: It
// knows the address of the register (within the MMIO address space) and
// the type of its contents, RegType.  RegType represents the register's
// bitfields.  RegType should be a subclass of RegisterBase.
template <class RegType> class RegisterAddr {
public:
    RegisterAddr(uint32_t reg_addr) : reg_addr_(reg_addr) {}

    static_assert(mxtl::is_base_of<RegisterBase<typename RegType::ValueType>, RegType>::value,
                  "Parameter of RegisterAddr<> should derive from RegisterBase");

    // Instantiate a RegisterBase using the value of the register read from
    // MMIO.
    RegType ReadFrom(RegisterIo* reg_io)
    {
        RegType reg;
        reg.set_reg_addr(reg_addr_);
        reg.ReadFrom(reg_io);
        return reg;
    }

    // Instantiate a RegisterBase using the given value for the register.
    RegType FromValue(typename RegType::ValueType value)
    {
        RegType reg;
        reg.set_reg_addr(reg_addr_);
        reg.set_reg_value(value);
        return reg;
    }

    uint32_t addr() { return reg_addr_; }

private:
    uint32_t reg_addr_;
};

namespace internal {

template <class IntType> constexpr IntType ComputeMask(uint32_t num_bits) {
    return static_cast<IntType>((static_cast<IntType>(1) << num_bits) - 1);
}

template <class IntType> class RsvdZField {
public:
    RsvdZField(IntType* mask, uint32_t bit_high_incl, uint32_t bit_low) {
        *mask |= internal::ComputeMask<IntType>(bit_high_incl - bit_low + 1) << bit_low;
    }
};

} // namespace internal

// TODO(teisenbe): Maybe get rid of this class and turn it into utility
// functions?
template <class IntType> class BitfieldRef {
public:
    BitfieldRef(IntType* value_ptr, uint32_t bit_high_incl, uint32_t bit_low)
        : value_ptr_(value_ptr), shift_(bit_low),
          mask_(internal::ComputeMask<IntType>(bit_high_incl - bit_low + 1))
    {
    }

    IntType get() { return (*value_ptr_ >> shift_) & mask_; }

    void set(IntType field_val)
    {
        MX_DEBUG_ASSERT((field_val & ~mask_) == 0);
        *value_ptr_ &= ~(mask_ << shift_);
        *value_ptr_ |= (field_val << shift_);
    }

private:
    IntType* const value_ptr_;
    const uint32_t shift_;
    const IntType mask_;
};

// Declares multi-bit fields in a derived class of RegisterBase<T>.  This
// produces functions "T NAME()" and "void set_NAME(T)".  Both bit indices
// are inclusive.
#define DEF_FIELD(BIT_HIGH, BIT_LOW, NAME)                                                         \
    static_assert((BIT_HIGH) > (BIT_LOW), "Upper bit goes before lower bit");                      \
    static_assert((BIT_HIGH) < sizeof(ValueType) * CHAR_BIT, "Upper bit is out of range");         \
    ValueType NAME()                                                                               \
    {                                                                                              \
        return hwreg::BitfieldRef<ValueType>(reg_value_ptr(), (BIT_HIGH), (BIT_LOW)).get();        \
    }                                                                                              \
    void set_ ## NAME(ValueType val)                                                               \
    {                                                                                              \
        hwreg::BitfieldRef<ValueType>(reg_value_ptr(), (BIT_HIGH), (BIT_LOW)).set(val);            \
    }

// Declares single-bit fields in a derived class of RegisterBase<T>.  This
// produces functions "T NAME()" and "void set_NAME(T)".
#define DEF_BIT(BIT, NAME)                                                                         \
    static_assert((BIT) < sizeof(ValueType) * CHAR_BIT, "Bit is out of range");                    \
    ValueType NAME()                                                                               \
    {                                                                                              \
        return hwreg::BitfieldRef<ValueType>(reg_value_ptr(), (BIT), (BIT)).get();                 \
    }                                                                                              \
    void set_ ## NAME(ValueType val)                                                               \
    {                                                                                              \
        hwreg::BitfieldRef<ValueType>(reg_value_ptr(), (BIT), (BIT)).set(val);                     \
    }

// Declares multi-bit reserved-zero fields in a derived class of RegisterBase<T>.
// This will ensure that on RegisterBase<T>::WriteTo(), reserved-zero bits are
// zeroed.  Both bit indices are inclusive.
#define DEF_RSVDZ_FIELD(BIT_HIGH, BIT_LOW)                                                         \
    static_assert((BIT_HIGH) > (BIT_LOW), "Upper bit goes before lower bit");                      \
    static_assert((BIT_HIGH) < sizeof(ValueType) * CHAR_BIT, "Upper bit is out of range");         \
    hwreg::internal::RsvdZField<ValueType> RsvdZ ## BIT_HIGH ## _ ## BIT_LOW =                     \
        hwreg::internal::RsvdZField<ValueType>(&this->rsvdz_mask_, (BIT_HIGH), (BIT_LOW));

// Declares single-bit reserved-zero fields in a derived class of RegisterBase<T>.
// This will ensure that on RegisterBase<T>::WriteTo(), reserved-zero bits are
// zeroed.
#define DEF_RSVDZ_BIT(BIT)                                                                         \
    static_assert((BIT) < sizeof(ValueType) * CHAR_BIT, "Bit is out of range");                    \
    hwreg::internal::RsvdZField<ValueType> RsvdZ ## BIT =                                          \
        hwreg::internal::RsvdZField<ValueType>(&this->rsvdz_mask_, (BIT), (BIT));

// Declares "decltype(FIELD) NAME()" and "void set_NAME(decltype(FIELD))" that
// reads/modifies the declared bitrange.  Both bit indices are inclusive.
#define DEF_SUBFIELD(FIELD, BIT_HIGH, BIT_LOW, NAME)                                               \
    typename mxtl::remove_reference<decltype(FIELD)>::type NAME()                                  \
    {                                                                                              \
        return hwreg::BitfieldRef<typename mxtl::remove_reference<decltype(FIELD)>::type>(         \
            &FIELD, (BIT_HIGH), (BIT_LOW)).get();                                                  \
    }                                                                                              \
    void set_ ## NAME(typename mxtl::remove_reference<decltype(FIELD)>::type val)                  \
    {                                                                                              \
        hwreg::BitfieldRef<typename mxtl::remove_reference<decltype(FIELD)>::type>(                \
                &FIELD, (BIT_HIGH), (BIT_LOW)).set(val);                                           \
    }

// Declares "decltype(FIELD) NAME()" and "void set_NAME(decltype(FIELD))" that
// reads/modifies the declared bit.
#define DEF_SUBBIT(FIELD, BIT, NAME)                                                               \
    typename mxtl::remove_reference<decltype(FIELD)>::type NAME()                                  \
    {                                                                                              \
        static_assert((BIT) <                                                                      \
                      sizeof(typename mxtl::remove_reference<decltype(FIELD)>::type) * CHAR_BIT,   \
                      "Bit is out of range");                                                      \
        return hwreg::BitfieldRef<typename mxtl::remove_reference<decltype(FIELD)>::type>(         \
                &FIELD, (BIT), (BIT)).get();                                                       \
    }                                                                                              \
    void set_ ## NAME(typename mxtl::remove_reference<decltype(FIELD)>::type val)                  \
    {                                                                                              \
        hwreg::BitfieldRef<typename mxtl::remove_reference<decltype(FIELD)>::type>(                \
                &FIELD, (BIT), (BIT)).set(val);                                                    \
    }

} // namespace hwreg
