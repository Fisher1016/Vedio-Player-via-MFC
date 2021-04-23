
// testMFCDlg.cpp: 实现文件
//
//统一的库
#define _CRT_SECURE_NO_WARNINGS     
#define __STDC_CONSTANT_MACROS
#define REFRESH_EVENT  (SDL_USEREVENT + 1)
#define BREAK_EVENT  (SDL_USEREVENT + 2)


#include "pch.h"
#include "framework.h"
#include "testMFC.h"
#include "testMFCDlg.h"
#include "afxdialogex.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#endif
#include <stdio.h>
extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "sdl2/SDL.h"
};
extern "C"
{
	FILE __iob_func[3] = { *stdin,*stdout,*stderr };
}

// 用于应用程序“关于”菜单项的 CAboutDlg 对话框
#define REFRESH_EVENT  (SDL_USEREVENT + 1)
#define BREAK_EVENT  (SDL_USEREVENT + 2)
class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CtestMFCDlg 对话框



CtestMFCDlg::CtestMFCDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_TESTMFC_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CtestMFCDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_URL, m_url);
}

BEGIN_MESSAGE_MAP(CtestMFCDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_PLAY, &CtestMFCDlg::OnBnClickedPlay)
	ON_BN_CLICKED(IDC_ABOUT, &CtestMFCDlg::OnBnClickedAbout)
	ON_BN_CLICKED(IDC_FILE, &CtestMFCDlg::OnBnClickedFile)
	ON_BN_CLICKED(IDC_CANCEL, &CtestMFCDlg::OnBnClickedCancel)
	ON_BN_CLICKED(IDC_WAIT, &CtestMFCDlg::OnBnClickedWait)
	ON_BN_CLICKED(IDC_STOP, &CtestMFCDlg::OnBnClickedStop)
END_MESSAGE_MAP()


// CtestMFCDlg 消息处理程序
const int bpp = 12;
int screen_w = 540, screen_h = 320;
const int pixel_w = 320, pixel_h = 180;
unsigned char buffer[pixel_w * pixel_h * bpp / 8];
int thread_exit = 0;
int thread_pause = 0;

//Refresh Event
#define REFRESH_EVENT  (SDL_USEREVENT + 1)

#define BREAK_EVENT  (SDL_USEREVENT + 2)
//更新
int refresh_video(void* opaque) {
	thread_exit = 0;
	thread_pause = 0;
	while (thread_exit == 0) {
		//SDL_Event event;
		//event.type = REFRESH_EVENT;
		//SDL_PushEvent(&event);
		if (!thread_exit) {
			SDL_Event event;
			event.type = REFRESH_EVENT;
			SDL_PushEvent(&event);
		}
		SDL_Delay(40);
	}
	//thread_exit = 0;
	//quit
	SDL_Event event;
	event.type = BREAK_EVENT;
	SDL_PushEvent(&event);
	thread_exit = 0;
	thread_pause = 0;
	return 0;
}

UINT ffmpegplayer(LPVOID lpParam)     //有个固定格式,第一次把参数类型写成了LPWORD，郁闷了半小时
{
	AVFormatContext* pFormatCtx;
	AVCodecContext* pCodecCtx;
	AVCodec* pCodec;
	AVFrame* pFrame, * pFrameYUV;
	uint8_t* out_buffer;
	AVPacket* packet;
	int	i, videoindex;
	int y_size;
	int ret, got_picture;
	struct SwsContext* img_convert_ctx;

	CtestMFCDlg* dig = (CtestMFCDlg*)lpParam;  //地址转为指针

	//---------------------------
	char filepath[500] = { 0 };
	GetWindowTextA(dig->m_url, (LPSTR)filepath, 500);
	//---------------------------
	//输入文件路径
	//char filepath[] = "nxn.mp4";
	//char* filepath = argv[1];


	int frame_cnt;
	av_register_all();
	avformat_network_init();
	pFormatCtx = avformat_alloc_context();

	//确认文件打开
	if (avformat_open_input(&pFormatCtx, filepath, NULL, NULL) != 0) {
		printf("Couldn't open input stream.\n");
		return -1;
	}
	if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
		printf("Couldn't find stream information.\n");
		return -1;
	}
	videoindex = -1;
	for (i = 0; i < pFormatCtx->nb_streams; i++)
		if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
			videoindex = i;
			break;
		}
	if (videoindex == -1) {
		printf("Didn't find a video stream.\n");
		return -1;
	}

	pCodecCtx = pFormatCtx->streams[videoindex]->codec;
	pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
	if (pCodec == NULL) {
		printf("Codec not found.\n");
		return -1;
	}
	if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
		printf("Could not open codec.\n");
		return -1;
	}
	pFrame = av_frame_alloc();
	pFrameYUV = av_frame_alloc();
	out_buffer = (uint8_t*)av_malloc(avpicture_get_size(PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height));
	avpicture_fill((AVPicture*)pFrameYUV, out_buffer, PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height);
	packet = (AVPacket*)av_malloc(sizeof(AVPacket));
	img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt,
		pCodecCtx->width, pCodecCtx->height, PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);
	frame_cnt = 0;



	//初始化
	if (SDL_Init(SDL_INIT_VIDEO| SDL_INIT_TIMER)) {
		printf("Could not initialize SDL - %s\n", SDL_GetError());
		return -1;
	}

	//建立窗口
	SDL_Window* screen;
	screen_w = pCodecCtx->width;
	screen_h = pCodecCtx->height;
//	screen = SDL_CreateWindow("Simplest Video Play SDL2", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
//		screen_w, screen_h, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
	screen = SDL_CreateWindowFrom(dig->GetDlgItem(IDC_SCREEN)->GetSafeHwnd());
	if (!screen) {
		printf("SDL: could not create window - exiting:%s\n", SDL_GetError());
		return -1;
	}

	//建立渲染器
	SDL_Renderer* sdlRenderer = SDL_CreateRenderer(screen, -1, 0);
	Uint32 pixformat = 0;
	pixformat = SDL_PIXELFORMAT_IYUV;

	//建立纹理
	SDL_Texture* sdlTexture = SDL_CreateTexture(sdlRenderer, pixformat, SDL_TEXTUREACCESS_STREAMING, pCodecCtx->width, pCodecCtx->height);
	SDL_Rect sdlRect;
	SDL_Thread* refresh_thread = SDL_CreateThread(refresh_video, NULL, NULL);
	SDL_Event event;

	while (av_read_frame(pFormatCtx, packet) >= 0) {
		//Wait
		SDL_WaitEvent(&event);
		if (event.type == REFRESH_EVENT) {
			while (1) {
				if (av_read_frame(pFormatCtx, packet) < 0)
					thread_exit = 1;        //如果是音频，不退出循环，直接读取

				if (packet->stream_index == videoindex)
					break;
			}
			if (packet->stream_index == videoindex) {
				/*
				 * 在此处添加输出H264码流的代码
				 * 取自于packet，使用fwrite()
				 */
				ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, packet);
				if (ret < 0) {
					printf("Decode Error.\n");
					return -1;
				}
				if (got_picture) {
					sws_scale(img_convert_ctx, (const uint8_t* const*)pFrame->data, pFrame->linesize, 0, pCodecCtx->height,
						pFrameYUV->data, pFrameYUV->linesize);
					printf("Decoded frame index: %d\n", frame_cnt);

					/*
					 * 在此处添加输出YUV的代码
					 * 取自于pFrameYUV，使用fwrite()
					 */
					SDL_UpdateTexture(sdlTexture, NULL, pFrameYUV->data[0], pFrameYUV->linesize[0]);

					//FIX: If window is resize
					sdlRect.x = 0;
					sdlRect.y = 0;
					sdlRect.w = screen_w;
					sdlRect.h = screen_h;

					SDL_RenderClear(sdlRenderer);
					SDL_RenderCopy(sdlRenderer, sdlTexture, NULL, &sdlRect);
					SDL_RenderPresent(sdlRenderer);

					frame_cnt++;

				}
			}
			av_free_packet(packet);
		}
		else if (event.type ==SDL_KEYDOWN) {
			//Pause
			if (event.key.keysym.sym == SDLK_SPACE)
				thread_pause = !thread_pause;
		}
		else if (event.type == SDL_QUIT) {
			thread_exit = 1;
		}
		else if (event.type == BREAK_EVENT) {
			break;
		}
	}

	sws_freeContext(img_convert_ctx);
	av_frame_free(&pFrameYUV);
	av_frame_free(&pFrame);
	avcodec_close(pCodecCtx);
	avformat_close_input(&pFormatCtx);
	return 0;
}

BOOL CtestMFCDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CtestMFCDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CtestMFCDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CtestMFCDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CtestMFCDlg::OnBnClickedPlay()
{	
	AfxBeginThread(ffmpegplayer,this);

}


void CtestMFCDlg::OnBnClickedAbout()
{
	CAboutDlg dig1;
	dig1.DoModal();
}




void CtestMFCDlg::OnBnClickedFile()
{
	CString FilePathName;
	CFileDialog dlg(TRUE, NULL, NULL, NULL, NULL);///TRUE为OPEN对话框，FALSE为SAVE AS对话框 
	if (dlg.DoModal() == IDOK) {
		FilePathName = dlg.GetPathName();
		m_url.SetWindowText(FilePathName);
	}
}


void CtestMFCDlg::OnBnClickedCancel()
{
	thread_exit = 1;
}


void CtestMFCDlg::OnBnClickedWait()
{
	//thread_pause = !thread_pause;
	SDL_PauseAudio(thread_pause);
}


void CtestMFCDlg::OnBnClickedStop()
{
	SDL_Quit();
}