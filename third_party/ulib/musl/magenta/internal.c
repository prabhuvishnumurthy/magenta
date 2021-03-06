#include "magenta_impl.h"

#undef _mx_process_self
#undef _mx_vmar_root_self
#undef _mx_job_default

#include <magenta/process.h>

mx_handle_t __magenta_process_self ATTR_LIBC_VISIBILITY;
mx_handle_t __magenta_vmar_root_self ATTR_LIBC_VISIBILITY;
mx_handle_t __magenta_job_default ATTR_LIBC_VISIBILITY;

mx_handle_t _mx_process_self(void) {
    return __magenta_process_self;
}
__typeof(mx_process_self) mx_process_self
    __attribute__((weak, alias("_mx_process_self")));

mx_handle_t _mx_vmar_root_self(void) {
    return __magenta_vmar_root_self;
}
__typeof(mx_vmar_root_self) mx_vmar_root_self
    __attribute__((weak, alias("_mx_vmar_root_self")));

mx_handle_t _mx_job_default(void) {
    return __magenta_job_default;
}
__typeof(mx_job_default) mx_job_default
    __attribute__((weak, alias("_mx_job_default")));
