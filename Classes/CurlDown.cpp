#include "CurlDown.h"
#include "io.h"

CurlDown::CurlDown()
{
	mFileName = "res.temp";
	mFilePath = FileUtils::getInstance()->getSearchPaths()[0]+"download/res.temp";
	mDownloadUrl = "http://127.0.0.1:8080/download";
	mFileLenth=0;
}

CurlDown::~CurlDown()
{
}

static string getSize(double size)
{
	string unit = "";
	char tsize[10]="";
	if (size > 1024 * 1024 * 1024)
	{
		unit = "G";
		size /= 1024 * 1024 * 1024;
	}
	else if (size > 1024 * 1024)
	{
		unit = "M";
		size /= 1024 * 1024;
	}
	else if (size > 1024)
	{
		unit = "KB";
		size /= 1024;
	}
	sprintf(tsize, "%.1f", size);//����1λС��(����������)
	string res = tsize + unit;
	return res;
}

static size_t write_func(void *ptr, size_t size, size_t nmemb, void *userdata) {
	FILE *fp = (FILE*)userdata;
	size_t written = fwrite(ptr, size, nmemb, fp);
	return written;
}

static size_t getcontentlengthfunc(void *ptr, size_t size, size_t nmemb, void *data)
{
	return (size_t)(size * nmemb);
}


/************************************************************************/
/*  
����ص��������Ը��������ж���������Ҫ�����Լ������˶������ݣ���λ���ֽڡ�
totalToDownload����Ҫ���ص����ֽ���(���ﲻ�������������ص�һ����)��nowDownloaded���Ѿ����ص��ֽ���(ָ����totalToDownload�������ض���)��
totalToUpLoad�ǽ�Ҫ�ϴ����ֽ�����nowUpLoaded���Ѿ��ϴ����ֽ��������������������ݵĻ�����ôultotal��ulnow������0����֮��
���������ϴ��Ļ�����ôdltotal��dlnowҲ����0��clientpΪ�û��Զ��������
ͨ������CURLOPT_XFERINFODATA���������ݡ��˺������ط�0ֵ�����жϴ��䣬���������CURLE_ABORTED_BY_CALLBACK 
*/
/************************************************************************/
static int progress_func(void *ptr, double totalToDownload, double nowDownloaded, double totalToUpLoad, double nowUpLoaded) {
	CurlDown* curlDown = (CurlDown*)ptr;
	CURL *easy_handle = curlDown->libcurl;

	char timeFormat[12] = "";
	double speed; //�����ٶ�
	string unit = "B";
	curl_easy_getinfo(easy_handle, CURLINFO_SPEED_DOWNLOAD, &speed); // curl_get_info������curl_easy_perform֮�����
	if (speed != 0)
	{
		// Time remaining  
		double leftTime = (curlDown->mFileLenth - nowDownloaded - curlDown->getLocalFileLength()) / speed; //���ܴ�С-�����ش�С-���ش洢��С��/�ٶ�
		int hours = leftTime / 3600;
		int minutes = (leftTime - hours * 3600) / 60;
		int seconds = leftTime - hours * 3600 - minutes * 60;
		sprintf(timeFormat, "%02d:%02d:%02d", hours, minutes, seconds);//ʣ��ʱ���ʽ��
	}

	if (speed > 1024 * 1024 * 1024)
	{
		unit = "G";
		speed /= 1024 * 1024 * 1024;
	}
	else if (speed > 1024 * 1024)
	{
		unit = "M";
		speed /= 1024 * 1024;
	}
	else if (speed > 1024)
	{
		unit = "KB";
		speed /= 1024;
	}	
	log("speed:%.2f%s/s", speed, unit.c_str());
	char fspeed[20]="";
	sprintf(fspeed, "%.2f%s/s", speed, unit.c_str());
	if (!curlDown || curlDown->mFileLenth == 0 || nowDownloaded == 0) return 0;
	double nowDown = (curlDown->mFileLenth - totalToDownload + nowDownloaded);
	double curpercent = nowDown / curlDown->mFileLenth * 100; 
	curlDown->onProgress(curpercent, getSize(curlDown->mFileLenth), getSize(curlDown->mFileLenth - totalToDownload + nowDownloaded), fspeed);
	log("nowDd:%d; totalDown:%d; downProgress:%.2f%%",(int)(curlDown->mFileLenth - totalToDownload + nowDownloaded),(int)curlDown->mFileLenth , curpercent);
		
	return 0;
}




void CurlDown::downStart()
{
	thread _st_d(&CurlDown::downloadControler, this);//����һ����֧�߳�
	_st_d.detach();
}

void CurlDown::downloadControler()
{
	/*mFileName = mDownloadUrl.substr(mDownloadUrl.rfind('/') + 1);*/
	mFileLenth = getDownloadFileLenth();
	log("file size================%f", mFileLenth);
	if (mFileLenth <= 0) {		
		return;
	}	
	bool ret = false;
	int timeout = 30;
	while (true) { // ѭ������ ÿ30��������� ����������
		ret = download(timeout); //ֱ������
		log("----ret--------->%d",ret);
		if (ret) { //�������
			break;
		}
		Sleep(500); //ÿ�����ؼ��500ms
	}

	if (ret)
	{		
		onSuccess(ret, mFilePath);
	}
}

bool CurlDown::download(long timeout)
{
	FILE *fp = NULL;
	if (_access(mFilePath.c_str(), 0) == 0) { // �Զ�������ʽ׷��
		fp = fopen(mFilePath.c_str(), "ab+");
	}
	else { // ������д
		fp = fopen(mFilePath.c_str(), "wb");
	}

	if (fp == NULL) {// ����ļ���ʼ��ʧ�ܽ��з���
		return false;
	}

	// ��ȡ�����ļ����ش�С
	int use_resume = 0;
	long localFileLenth = getLocalFileLength(); //�Ѿ����صĴ�С
	if (localFileLenth>0)
	{
		use_resume = 1;
	}
	log("filePath:%s  ; leng:%ld", mFilePath.c_str(), localFileLenth);
	CURL *handle = curl_easy_init();
	libcurl = handle;
	curl_easy_setopt(handle, CURLOPT_URL, mDownloadUrl.c_str()); 
//	curl_easy_setopt(handle, CURLOPT_TIMEOUT, timeout);//��������ʱ�䣬����ʱ��Ͽ����أ�����ģʽ�°�����ע�Ϳ�����ʱ����������
//	curl_easy_setopt(handle, CURLOPT_CONNECTTIMEOUT, 0);//�о�ûʲô����,�ÿ���
	curl_easy_setopt(handle, CURLOPT_HEADERFUNCTION, getcontentlengthfunc);
	curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, write_func);   //д�ļ��ص�����
	curl_easy_setopt(handle, CURLOPT_WRITEDATA, fp); // д���ļ�����
    curl_easy_setopt(handle, CURLOPT_RESUME_FROM, use_resume ? localFileLenth : 0);  // �ӱ��ش�Сλ�ý�����������
//	curl_easy_setopt(handle, CURLOPT_RESUME_FROM_LARGE, (long long)(use_resume ? localFileLenth : 0)); // CURLOPT_RESUME_FROM_LARGE����ڴ��ļ�
	curl_easy_setopt(handle, CURLOPT_NOPROGRESS, 0L);
	curl_easy_setopt(handle, CURLOPT_PROGRESSFUNCTION, progress_func); //���ؽ��Ȼص�����
	curl_easy_setopt(handle, CURLOPT_PROGRESSDATA, this); // ���뱾�����
	//curl_easy_setopt(handle, CURLOPT_PROGRESSDATA, handle);//��handle����

	CURLcode res = curl_easy_perform(handle);
	fclose(fp);

	return res == CURLE_OK;
}

long CurlDown::getDownloadFileLenth()
{
	CURL *handle = curl_easy_init();
	curl_easy_setopt(handle, CURLOPT_URL, mDownloadUrl.c_str());
	//curl_easy_setopt(handle, CURLOPT_HEADER, 1); //���ʹlibcurl��Ĭ�����û�ȡ��ʽΪHEAD��ʽ�������set nobody��optionȥ�����ֻ������ļ����ݣ�����ȡ
	curl_easy_setopt(handle, CURLOPT_CUSTOMREQUEST, "GET"); //����Ϊget��ʽ��ȡ
	curl_easy_setopt(handle, CURLOPT_NOBODY, 1);
	CURLcode retcCode = curl_easy_perform(handle);
	//�鿴�Ƿ��г�����Ϣ  
	const char* pError = curl_easy_strerror(retcCode);
	log("error===========================%s",pError);
	//����curl����ǰ��ĳ�ʼ��ƥ��  
	curl_easy_cleanup(handle);
	if (retcCode == CURLE_OK) {
		curl_easy_getinfo(handle, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &mFileLenth);
		log("filesize : %0.0f bytes\n", mFileLenth);
	}else {
		mFileLenth = -1;
	}
	return mFileLenth;
}

long CurlDown::getLocalFileLength()
{
	FILE *fp = fopen(mFilePath.c_str(), "r");
	fseek(fp, 0, SEEK_END);
	long length = ftell(fp);
	fclose(fp);
	return length;
}


