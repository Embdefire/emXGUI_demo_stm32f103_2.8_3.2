#include <emXGUI.h>
#include <string.h>
#include "Widget.h"
#include "bsp_ov7725.h"
#include "x_libc.h"




TaskHandle_t h_autofocus;
BOOL update_flag = 0;//֡�ʸ��±�־
uint8_t fps=0;//֡��
HWND Cam_hwnd;//�����ھ��
static SURFACE *pSurf;
GUI_SEM *cam_sem = NULL;//����ͼ��ͬ���ź�������ֵ�ͣ�
uint16_t *cam_buff;

/*
 * @brief  ��һ��֡ͼ��
 * @param  ͼ��Ļ�����
 * @retval NONE
*/
void OV7725_Read_Frame(uint16_t *p)
{
  /* �����ٽ�Σ��ٽ�ο���Ƕ�� */
//  taskENTER_CRITICAL();
  
  for(int i = 0; i < 240*320; i++)
  {
    READ_FIFO_PIXEL(*p);		/* ��FIFO����һ�� rgb565 ���ص� p ���� */
    p++;
  }
  
//  taskEXIT_CRITICAL();
}

/*
 * @brief  ������Ļ
 * @param  NONE
 * @retval NONE
*/
static void Update_Dialog()
{
  /* ov7725 ���ź��߳�ʼ�� */
  Ov7725_vsync = 0;
  VSYNC_Init();
  
	while(1) //�߳��Ѵ�����
	{
    GUI_SemWait(cam_sem, 0xFFFFFFFF);
  
    FIFO_PREPARE;  			/*FIFO׼��*/

    OV7725_Read_Frame(cam_buff);    // ��һ֡ͼ��
    fps ++;                         // ֡���Լ�

    InvalidateRect(Cam_hwnd,NULL,FALSE);
	}
}

/*
 * @brief  ����ͷ���ڻص�����
*/
static LRESULT WinProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  static uint8_t OV7725_State = 0;    // 0:���Լ�⵽����ͷ
  static int old_fps = 0;
  
  switch(msg)
  {
    case WM_CREATE:
    {
      /* ov7725 gpio ��ʼ�� */
      Ov7725_GPIO_Config();

      if(Ov7725_Init())
      {
        GUI_DEBUG("OV7725 ��ʼ���ɹ�");
        OV7725_State = 0;
      }
      else
      {
        MSGBOX_OPTIONS ops;
        //const WCHAR *btn[]={L"ȷ��"};
        int x,y,w,h;

        ops.Flag =MB_ICONERROR;
        //ops.pButtonText =btn;
        ops.ButtonCount =0;
        w =200;
        h =120;
        x =(GUI_XSIZE-w)>>1;
        y =(GUI_YSIZE-h)>>1;
        MessageBox(hwnd,x,y,w,h,L"û�м�⵽OV7725����ͷ��\n�����¼�����ӡ�",L"����",&ops); 
        OV7725_State = 1;     // û�м�⵽����ͷ
        PostCloseMessage(hwnd);
        break;  
      }     
      
      pSurf =CreateSurface(SURF_RGB565,320, 240, 0, (U16 *)cam_buff);   
      cam_sem = GUI_SemCreate(0,1);//ͬ������ͷͼ��
      
      //�����Զ��Խ��߳�
      xTaskCreate((TaskFunction_t )(void(*)(void*))Update_Dialog,  /* ������ں��� */
                            (const char*    )"Update_Dialog",/* �������� */
                            (uint16_t       )1*1024/4,  /* ����ջ��СFreeRTOS������ջ����Ϊ��λ */
                            (void*          )NULL,/* ������ں������� */
                            (UBaseType_t    )5, /* ��������ȼ� */
                            (TaskHandle_t  )&h_autofocus);/* ������ƿ�ָ�� */

      
      SetTimer(hwnd,1,999,TMR_START,NULL);  

      break;  
    }
    case WM_LBUTTONDOWN://�����Ļ���رմ���
    {
      
      PostCloseMessage(hwnd);
  
      break;
    }
    case WM_TIMER://����ͷ״̬��
    {
      update_flag = 1;

      break;
    }
    case WM_PAINT:
    {
      PAINTSTRUCT ps;
//      SURFACE *pSurf;
      HDC hdc_mem;
      HDC hdc;
      WCHAR wbuf[20];
      RECT rc;
      
      hdc = BeginPaint(hwnd,&ps);
      GetClientRect(hwnd,&rc);
      if(OV7725_State == 1)
      {
        SetTextColor(hdc,MapRGB(hdc,250,250,250));
        SetBrushColor(hdc,MapRGB(hdc,50,0,0));
        SetPenColor(hdc,MapRGB(hdc,250,0,0));
        
        DrawText(hdc,L"���ڳ�ʼ������ͷ\r\n\n��ȴ�...",-1,&rc,DT_VCENTER|DT_CENTER|DT_BKGND);
      }              
      if(OV7725_State == 0)
      {   
        
        hdc_mem =CreateDC(pSurf,NULL);
        BitBlt(hdc, 0, 0, 320, 240, hdc_mem, 0 , 0, SRCCOPY);

        DeleteDC(hdc_mem);
      }
#if 0
      // ���´��ڷֱ���
      if(update_flag)
      {
        update_flag = 0;
        old_fps = fps;
        fps = 0;
      }
      
      rc.x = 0;
      rc.y = 0;
      rc.w = 70;
      rc.h = 15;
      
      x_wsprintf(wbuf,L"֡��:%dFPS",old_fps);
      DrawText(hdc,wbuf,-1,&rc,DT_VCENTER|DT_CENTER);
#endif
      EndPaint(hwnd,&ps);
      Ov7725_vsync = 0;    // ��ʼ��һ֡ͼ��Ĳɼ�
      break;
    }

    case WM_DESTROY:
    {
      old_fps = 0;
      fps = 0;
      DeleteSurface(pSurf);

      if (!OV7725_State)
      {
        GUI_Thread_Delete(h_autofocus);
      }

      GUI_VMEM_Free(cam_buff);
      discameraexit();             // �ر��ж�
      GUI_SemDelete(cam_sem);
      return PostQuitMessage(hwnd);	
    }    
    default:
      return DefWindowProc(hwnd, msg, wParam, lParam);
  }
  return WM_NULL;
}


void	GUI_Camera_DIALOG(void)
{	
	WNDCLASS	wcex;
	MSG msg;

	wcex.Tag = WNDCLASS_TAG;  
  
  cam_buff = (uint16_t *)GUI_VMEM_Alloc(LCD_XSIZE*LCD_YSIZE*2);
  
	wcex.Style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WinProc; //������������Ϣ����Ļص�����.
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = NULL;//hInst;
	wcex.hIcon = NULL;//LoadIcon(hInstance, (LPCTSTR)IDI_WIN32_APP_TEST);
	wcex.hCursor = NULL;//LoadCursor(NULL, IDC_ARROW);

	//����������
	Cam_hwnd = CreateWindowEx(WS_EX_NOFOCUS,
                                    &wcex,
                                    L"GUI_Camera_Dialog",
                                    WS_VISIBLE|WS_CLIPCHILDREN|WS_OVERLAPPED,
                                    0, 0, GUI_XSIZE, GUI_YSIZE,
									NULL, NULL, NULL, NULL);

	//��ʾ������
	ShowWindow(Cam_hwnd, SW_SHOW);

	//��ʼ������Ϣѭ��(���ڹرղ�����ʱ,GetMessage������FALSE,�˳�����Ϣѭ��)��
	while (GetMessage(&msg, Cam_hwnd))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
  }


}
