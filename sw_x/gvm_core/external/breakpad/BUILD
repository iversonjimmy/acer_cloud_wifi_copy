external_static_cpp_lib(
    name = 'libbreakpad',
    # Note that includes are relative to $(PBROOT) for external_* makegen targets:
    includes = [
            'usr/include',
            'gvm_core/external/breakpad/google-breakpad-read-only/src'
            ],
    outfile = 'usr/lib/libbreakpad_client.a'
)

# TODO: Do we want a shared lib version?
#external_shared_cpp_lib(
#    name = 'libbreakpad-shr',
#    # Note that includes are relative to $(PBROOT) for external_* makegen targets:
#    includes = [ # Note that these are relative to $(PBROOT):
#            'usr/include',
#            'gvm_core/external/breakpad/google-breakpad-read-only/src'
#            ],
#    outfile = 'usr/lib/libbreakpad_client.so'
#)
