// Copyright 2017 The Fuchsia Authors
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT

#include <dev/iommu/dummy.h>

#include <err.h>
#include <kernel/vm.h>
#include <mxalloc/new.h>
#include <mxtl/ref_ptr.h>

DummyIommu::DummyIommu(uint64_t id) : Iommu(id) {
}

mxtl::RefPtr<Iommu> DummyIommu::Create(uint64_t id) {
    AllocChecker ac;
    auto instance = mxtl::AdoptRef<DummyIommu>(new (&ac) DummyIommu(id));
    if (!ac.check()) {
        return nullptr;
    }
    RegisterIommu(instance);
    return instance;
}

DummyIommu::~DummyIommu() {
}

bool DummyIommu::IsValidBusTxnId(uint64_t bus_txn_id) const {
    return true;
}

status_t DummyIommu::Map(uint64_t bus_txn_id, paddr_t paddr, size_t size, uint32_t perms,
                         dev_vaddr_t* vaddr) {
    DEBUG_ASSERT(vaddr);
    if (!IS_PAGE_ALIGNED(paddr) || !IS_PAGE_ALIGNED(size)) {
        return ERR_INVALID_ARGS;
    }
    if (perms & ~(IOMMU_FLAG_PERM_READ | IOMMU_FLAG_PERM_WRITE | IOMMU_FLAG_PERM_EXECUTE)) {
        return ERR_INVALID_ARGS;
    }
    if (perms == 0) {
        return ERR_INVALID_ARGS;
    }
    return NO_ERROR;
}

status_t DummyIommu::Unmap(uint64_t bus_txn_id, dev_vaddr_t vaddr, size_t size) {
    if (!IS_PAGE_ALIGNED(vaddr) || !IS_PAGE_ALIGNED(size)) {
        return ERR_INVALID_ARGS;
    }
    return NO_ERROR;
}

status_t DummyIommu::ClearMappingsForBusTxnId(uint64_t bus_txn_id) {
    return NO_ERROR;
}
