// dllmain.h : Declaration of module class.

class Cshellext_winModule : public CAtlDllModuleT< Cshellext_winModule >
{
public :
	DECLARE_LIBID(LIBID_shellext_winLib)
	DECLARE_REGISTRY_APPID_RESOURCEID(IDR_SHELLEXT_WIN, "{3B5E2723-A933-47A5-B2D8-5DE7F0C0736B}")
};

extern class Cshellext_winModule _AtlModule;
