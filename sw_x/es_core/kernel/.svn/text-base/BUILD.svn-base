cpp_binary(
	name = 'test_decrypt_content',
	includes = [
		'include',
		'../esc/include',
	],
	srcs = [
		'test/test_decrypt_content.c',
	],
	deps = [
		'/es_core:libescore',
		'/gvm_core/vpl:libvpl_minimal',
	],
	ldflags = '-m32'
)

cpp_binary(
	name = 'test_decrypt_secret_cred',
	includes = [
		'include',
		'../esc/include',
	],
	srcs = [
		'test/test_decrypt_secret_cred.c',
	],
	deps = [
		'/es_core:libescore',
		'/gvm_core/vpl:libvpl_minimal',
	],
	ldflags = '-m32'
)

kernel_module(
	name = 'escmod',
	srcdir = '/es_core/kernel/linux',
	outfile = 'escmod/escmod.ko',
)
