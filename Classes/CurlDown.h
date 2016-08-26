#pragma once
#ifndef __download__CurlDown__
#define __download__CurlDown__
#include <string>
#include "cocos2d.h"
#include <thread>
#include "../external/curl/include/win32/curl/curl.h"
using namespace std;
USING_NS_CC;
#pragma comment(lib, "libcurl_imp.lib")

/************************************************************************/
/* libcurl download file                                                */
/************************************************************************/
class CurlDown 
{
public:
	CurlDown();
	~CurlDown();
	//��ں���
	void downStart();
	//���ؿ���
	void downloadControler();
	//����
	bool download(long timeout);
	// ��ȡԶ�������ļ��Ĵ�С
	long getDownloadFileLenth();
	// ��ȡ�����ļ������ش�С
	long getLocalFileLength();

	virtual void onProgress(double percent, string totalSize, string downSize,string speed) {};
	virtual void onSuccess(bool isSuccess, string filefullPath) { rename(filefullPath.c_str(), (mFilePath.substr(0, mFilePath.find(mFileName))+"res.zip").c_str()); };
	
public:
	string mFilePath; // ���ش洢��ַ
	double mFileLenth; // �����ļ���С
	string mDownloadUrl; // ����URL
	CURL *libcurl;

private:
	string mFileName; // �����ļ�����
	bool isStop;
	
};

#endif