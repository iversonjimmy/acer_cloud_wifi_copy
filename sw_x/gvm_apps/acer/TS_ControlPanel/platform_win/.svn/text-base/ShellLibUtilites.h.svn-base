#pragma once

ref class ShellLibUtilites
{
public:
	ShellLibUtilites(void);

	static System::String^ EXT_NAME = L"library-ms";

    /// <summary>
    /// Get all names of libraries 
    /// </summary>
public: 
	static System::Collections::Generic::List<System::String^>^ GetAllLibraryNames()
    {
		System::String^ librariesPath = System::IO::Path::Combine(
											System::Environment::GetFolderPath(System::Environment::SpecialFolder::ApplicationData),
											Microsoft::WindowsAPICodePack::Shell::ShellLibrary::LibrariesKnownFolder->RelativePath);

		array<System::String^>^fileEntries = System::IO::Directory::GetFiles(librariesPath, "*." + EXT_NAME);
		System::Collections::IEnumerator^ files = fileEntries->GetEnumerator();

		System::Collections::Generic::List<System::String^>^ libList = gcnew System::Collections::Generic::List<System::String^>();

	   while ( files->MoveNext() )
	   {
		   libList->Add(safe_cast<System::String^>(  System::IO::Path::GetFileNameWithoutExtension( files->Current->ToString())));
		  
	   }
        return libList;
    }

	static System::Collections::Generic::List<System::String^>^ GetAllAttachedFolders(System::String^ szLibraryName)
	{
		System::Collections::Generic::List<System::String^>^ Folders = gcnew System::Collections::Generic::List<System::String^>();
		try
		{
			Microsoft::WindowsAPICodePack::Shell::ShellLibrary^ shellLibrary = Microsoft::WindowsAPICodePack::Shell::ShellLibrary::Load(szLibraryName, true);
			System::Collections::IEnumerator^ folders = shellLibrary->GetEnumerator();


		   while ( folders->MoveNext() )
		   {
				Microsoft::WindowsAPICodePack::Shell::ShellFileSystemFolder^ folder =
							(safe_cast<Microsoft::WindowsAPICodePack::Shell::ShellFileSystemFolder^>(folders->Current));

				Folders->Add(folder->Path);
		   }
		}
		catch(System::Exception^ e) 
		{ 
			;
		}

		return Folders;
	}


};
