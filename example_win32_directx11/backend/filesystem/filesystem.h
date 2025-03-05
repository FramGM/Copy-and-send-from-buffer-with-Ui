#pragma once
#include <string>
#include <vector>

enum InstanceErrors : int {
	OK,
	MESSAGES_PATH,
	IMAGES_PATH,
	PROGRAM_PATH
};

class C_Filesystem
{
public:
	int Instance();
	std::wstring getProgramFolder();
	const std::wstring getMessagesPath() { return m_sMessagesPath; };
	const std::wstring getImagesPath() { return m_sImagesPath; };

	bool deleteImage();
	std::wstring downloadImage(int imgIdx);

	const std::wstring getProgramPath() { return m_sProgramFolder; };

private:

	std::wstring m_sMessagesPath;
	std::wstring m_sImagesPath;

	std::wstring m_sProgramFolder;

};

inline C_Filesystem* g_Filesystem = new C_Filesystem();