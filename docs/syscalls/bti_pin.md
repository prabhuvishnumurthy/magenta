# mx_bti_pin

## NAME

bti_pin - pin pages and grant devices access to them

## SYNOPSIS

```
#include <magenta/syscalls.h>

mx_status_t mx_bti_pin(mx_handle_t bti, mx_handle_t vmo, uint64_t offset,
                       uint64_t size, uint32_t perms, uint64_t* addrs,
                       uint32_t addrs_len, uint32_t* actual_addrs_len);
```

## DESCRIPTION

**bti_pin**() pins pages of a VMO (i.e. prevents them from being decommitted)
and grants the hardware transaction ID represented by the BTI the ability to
access these pages, with the permissions specified in *perms*.

*offset* and *size* must be aligned to page boundaries.  *perms* is a bitfield
that may contain one or more of *MX_VM_FLAG_PERM_READ*, *MX_VM_FLAG_PERM_WRITE*,
and *MX_VM_FLAG_PERM_EXECUTE*.  In order for the call to succeed, *vmo* must
have the READ/WRITE/EXECUTE rights corresponding to the flags set in *perms*.

*addrs* will be populated with the device-physical addresses of the requested
VMO pages.  These addresses may be given to devices that issue memory
transactions with the hardware transaction ID associated with the BTI.  Exactly
one address is returned for each page in the VMO range, so *addrs_len* should be
*size*/PAGE_SIZE.  If the VMO was allocated as contiguous memory, it is guaranteed
that the returned addresses will be contiguous.

## RETURN VALUE

On success, **bti_pin**() returns NO_ERROR.  The device-physical addresses of the
requested VMO pages will be written in *addrs* and the number of entries will
be written in *actual_addrs_len*.

In the event of failure, a negative error value is returned.

## ERRORS

**ERR_BAD_HANDLE**  *bti* or *vmo* is not a valid handle.

**ERR_WRONG_TYPE**  *bti* is not a BTI handle or *vmo* is not a VMO handle.

**ERR_ACCESS_DENIED** *bti* or *vmo* does not have the *MX_RIGHT_MAP*, or
*perms* contained a flag corresponding to a right that *vmo* does not have.

**ERR_NOT_FOUND**  The requested range contains at least one uncommitted page.

**ERR_BUFFER_TOO_SMALL** *addrs* is not big enough to hold the result.

**ERR_INVALID_ARGS** *perms* is 0 or contains an undefined flag, or *addrs* or
*actual_addrs_len* is not a valid pointer.

**ERR_ALREADY_BOUND** The requested range contains a page already pinned by this
BTI.

**ERR_UNAVAILABLE** (Temporary) At least one page in the requested range could
not be pinned at this time.

**ERR_NO_MEMORY**  (Temporary) Failure due to lack of memory.

## SEE ALSO

[bti_create](bti_create.md),
[bti_unpin](bti_unpin.md).
