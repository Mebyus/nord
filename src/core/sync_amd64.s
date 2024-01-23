.global coven_sync_spin

// fn spin(cycles: u16)
//
//  [cycles] => rdi
coven_sync_spin:
	// Number of cycles comes from first argument by
    // calling convention directly into rdi register
again:
	pause
	dec	%di
	jnz	again
	ret
