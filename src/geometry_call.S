.text
.global geometry_filter_native
.hidden geometry_filter_native
.type geometry_filter_native, %function
geometry_filter_native:
    // The native five-byte aggregate uses the indirect-result register.
    mov x8, x2
    br x3
