#include <emXGUI.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include "ff.h"
#include "x_libc.h"
#include "GUI_AppDef.h"
#include "emXGUI_JPEG.h"
#include "emxgui_png.h"
#include  "GUI_ADC_CollectVoltage_Dialog.h"


/* ���� ID */
#define ID_ADV_WIN         0x01    // �м���ʾ����ID
#define SCROLLBAR_Brigh_ID 0x02    // ��������ID
#define ID_TEXTBOX_Title   0x03    // ������
#define ID_TEXTBOX_Brigh   0x04    // ���Ȱٷֱ�

#define CircleCenter_1    (79)     // ��������ת�뾶
#define CircleCenter_2    (100)    // Բ���������뾶��С��
#define CircleCenter_3    (CircleCenter_2 + 10)    //  ������ CircleSize / 2

/* �ƶ������־ */
#define LeftToRight    0
#define RightToLeft    1
#define MOVE_WIN       1

#define CircleSize    240    // Բ����ʾ����Ĵ�С
#define Circle_X      500    // Բ����ʾ�����λ��
#define Circle_Y      (50)   // Բ����ʾ�����λ��

#define GUI_ADC_BACKGROUNG_PIC      "musicdesktop.jpg"

#define TitleHeight    70    // �������ĸ߶�

#define TriangleLen    20    // �����εı߳�

uint8_t AovingDirection = 0;
double count = 0.0;
HWND MAIN_Handle;
HWND Title_Handle;
HWND Brigh_Handle;
HWND ADC_Handle;

HDC bk_hdc;

static COLORREF color_bg;//͸���ؼ��ı�����ɫ

// �ֲ����������ڱ���ת�������ĵ�ѹֵ 	 
double ADC_Vol; 

static void	X_MeterPointer(HDC hdc, int cx, int cy, int r, u32 color, double dat_val)
{
  double angle;
  int midpoint_x,midpoint_y;
  POINT pt[4];

  angle = (dat_val * 1.427 + 0.785);    // ����Ƕ�
  
  /* ����������ƽ�е�һ�ߵ��е����� */
  midpoint_x =cx - sin(angle) * (r - TriangleLen * 0.866);    // 0.866 = sqrt(3) / 2
  midpoint_y =cy + cos(angle) * (r - TriangleLen * 0.866);

  /* ��Զ��һ���� */
  pt[0].x = cx - r * sin(angle);
  pt[0].y = cy + r * cos(angle);

  /* ��Զһ������һ�� */
  pt[1].x = midpoint_x - (TriangleLen / 2) * sin(angle - 1.57);    // 1.57 = 3.14/2 = ��/2 = 90��
  pt[1].y = midpoint_y + (TriangleLen / 2) * cos(angle - 1.57);

  /* ��Զһ����ұ�һ�� */
  pt[2].x = midpoint_x - (TriangleLen / 2) * sin(angle + 1.57);
  pt[2].y = midpoint_y + (TriangleLen / 2) * cos(angle + 1.57);

  pt[3].x = pt[0].x;
  pt[3].y = pt[0].y;


  /* �������� */
  SetBrushColor(hdc,color);
  EnableAntiAlias(hdc, TRUE);
  FillPolygon(hdc,0,0,pt,4);
  EnableAntiAlias(hdc, FALSE);
}

//�˳���ť�ػ���
static void CollectVoltage_ExitButton_OwnerDraw(DRAWITEM_HDR *ds)
{
	HWND hwnd;
  HDC hdc;
	RECT rc;
 // RECT rc_top={0,0,800,70};
	WCHAR wbuf[128];

	hwnd = ds->hwnd; 
	hdc = ds->hDC;   
	rc = ds->rc; 

	SetBrushColor(hdc, MapRGB(hdc, COLOR_DESKTOP_BACK_GROUND));
   
  FillCircle(hdc, rc.x+rc.w, rc.y, rc.w);
	//FillRect(hdc, &rc); //�þ�����䱳��

  if (ds->State & BST_PUSHED)
	{ //��ť�ǰ���״̬
		SetTextColor(hdc, MapRGB(hdc, 105, 105, 105));      //��������ɫ
	}
	else
	{ //��ť�ǵ���״̬

		SetTextColor(hdc, MapRGB(hdc, 255, 255, 255));
	}

	  /* ʹ�ÿ���ͼ������ */
	SetFont(hdc, controlFont_64);

	GetWindowText(hwnd, wbuf, 128); //��ð�ť�ؼ�������
  rc.y = -10;
  rc.x = 16;
	DrawText(hdc, wbuf, -1, &rc, NULL);//��������(���ж��뷽ʽ)

  /* �ָ�Ĭ������ */
	SetFont(hdc, defaultFont);

}

/*
 * @brief  ���ƹ�����
 * @param  hwnd:   �������ľ��ֵ
 * @param  hdc:    ��ͼ������
 * @param  back_c��������ɫ
 * @param  Page_c: ������Page������ɫ
 * @param  fore_c���������������ɫ
 * @retval NONE
*/
static void draw_scrollbar(HWND hwnd, HDC hdc, COLOR_RGB32 back_c, COLOR_RGB32 Page_c, COLOR_RGB32 fore_c)
{
	RECT rc,rc_tmp;
   RECT rc_scrollbar;

	/* ���� */
   GetClientRect(hwnd, &rc_tmp);//�õ��ؼ���λ��
   GetClientRect(hwnd, &rc);//�õ��ؼ���λ��
   WindowToScreen(hwnd, (POINT *)&rc_tmp, 1);//����ת��
   
   BitBlt(hdc, rc.x, rc.y, rc.w, rc.h, bk_hdc, rc_tmp.x, rc_tmp.y, SRCCOPY);

   rc_scrollbar.x = rc.x;
   rc_scrollbar.y = rc.h/2-15;
   rc_scrollbar.w = rc.w;
   rc_scrollbar.h = 30;
   
	SetBrushColor(hdc, MapRGB888(hdc, Page_c));
	FillRect(hdc, &rc_scrollbar);
}

/*
 * @brief  ���ƹ�����
 * @param  hwnd:   �������ľ��ֵ
 * @param  hdc:    ��ͼ������
 * @param  back_c��������ɫ
 * @param  Page_c: ������Page������ɫ
 * @param  fore_c���������������ɫ
 * @retval NONE
*/
static void draw_gradient_scrollbar(HWND hwnd, HDC hdc, COLOR_RGB32 back_c, COLOR_RGB32 Page_c, COLOR_RGB32 fore_c)
{
	RECT rc,rc_tmp;
   RECT rc_scrollbar;
	GetClientRect(hwnd, &rc);
	/* ���� */
   GetClientRect(hwnd, &rc_tmp);//�õ��ؼ���λ��
   GetClientRect(hwnd, &rc);//�õ��ؼ���λ��
   WindowToScreen(hwnd, (POINT *)&rc_tmp, 1);//����ת��
   
   BitBlt(hdc, rc.x, rc.y, rc.w, rc.h, bk_hdc, rc_tmp.x, rc_tmp.y, SRCCOPY);

   rc_scrollbar.x = rc.x;
   rc_scrollbar.y = rc.h/2-15;
   rc_scrollbar.w = rc.w;
   rc_scrollbar.h = 30;
   
//	SetBrushColor(hdc, MapRGB888(hdc, Page_c));
	GradientFillRect(hdc, &rc_scrollbar, RGB888(175, 150, 150), RGB888( 255, 255, 255), FALSE);

}

/*
 * @brief  �Զ��廬�������ƺ���
 * @param  ds:	�Զ�����ƽṹ��
 * @retval NONE
*/
static void scrollbar_owner_draw(DRAWITEM_HDR *ds)
{
	HWND hwnd;
	HDC hdc;
	HDC hdc_mem;
	HDC hdc_mem1;
	RECT rc;
	RECT rc_cli;
	//	int i;

	hwnd = ds->hwnd;
	hdc = ds->hDC;
	GetClientRect(hwnd, &rc_cli);

	hdc_mem = CreateMemoryDC(SURF_SCREEN, rc_cli.w, rc_cli.h);
	hdc_mem1 = CreateMemoryDC(SURF_SCREEN, rc_cli.w, rc_cli.h);   
         
   	
	//���ư�ɫ���͵Ĺ�����
	draw_scrollbar(hwnd, hdc_mem1, color_bg, RGB888( 50, 50, 50), RGB888( 255, 255, 255));
	//���ƽ������͵Ĺ�����
	draw_gradient_scrollbar(hwnd, hdc_mem, color_bg, RGB888(	50, 50, 50), RGB888(50, 205, 50));
  SendMessage(hwnd, SBM_GETTRACKRECT, 0, (LPARAM)&rc);   

	//��
	BitBlt(hdc, rc_cli.x, rc_cli.y, rc.x + rc.w / 2, rc_cli.h, hdc_mem, 0, 0, SRCCOPY);
	//��
	BitBlt(hdc, rc.x + rc.w/2, rc_cli.y, rc_cli.w - (rc.x + rc.w/2) , rc_cli.h, hdc_mem1, rc.x + rc.w/2, 0, SRCCOPY);

	//���ƻ���
	if (ds->State & SST_THUMBTRACK)//����
	{
    /* ���� */
    SendMessage(hwnd, SBM_GETTRACKRECT, 0, (LPARAM)&rc);

    SetBrushColor(hdc, MapRGB(hdc, 169, 169, 169));

    /* �߿� */
    FillCircle(hdc, rc.x + rc.w / 2, rc.y + rc.h / 2, rc.h / 2 - 1);
    InflateRect(&rc, -2, -2);

    SetBrushColor(hdc, MapRGB888(hdc, RGB888(200, 200, 200)));
    FillCircle(hdc, rc.x + rc.w / 2, rc.y + rc.h / 2, rc.h / 2 - 1);
	}
	else//δѡ��
	{
		/* ���� */
    SendMessage(hwnd, SBM_GETTRACKRECT, 0, (LPARAM)&rc);

    SetBrushColor(hdc, MapRGB(hdc, 169, 169, 169));

    /* �߿� */
    FillCircle(hdc, rc.x + rc.w / 2, rc.y + rc.h / 2, rc.h / 2 - 1);
    InflateRect(&rc, -2, -2);

    SetBrushColor(hdc, MapRGB888(hdc, RGB888( 255, 255, 255)));
    FillCircle(hdc, rc.x + rc.w / 2, rc.y + rc.h / 2, rc.h / 2 - 1);
	}
  
	//�ͷ��ڴ�MemoryDC
	DeleteDC(hdc_mem1);
	DeleteDC(hdc_mem);
}

/*
 * @brief  �ػ�͸���ı�
 * @param  ds:	�Զ�����ƽṹ��
 * @retval NONE
*/
static void Textbox_OwnerDraw(DRAWITEM_HDR *ds) //����һ����ť���
{
	HWND hwnd;
	HDC hdc;
	RECT rc, rc_tmp;
	WCHAR wbuf[128];

	hwnd = ds->hwnd; //button�Ĵ��ھ��.
	hdc = ds->hDC;   //button�Ļ�ͼ�����ľ��.
  GetClientRect(hwnd, &rc_tmp);//�õ��ؼ���λ��
  GetClientRect(hwnd, &rc);//�õ��ؼ���λ��
  WindowToScreen(hwnd, (POINT *)&rc_tmp, 1);//����ת��

  BitBlt(hdc, rc.x, rc.y, rc.w, rc.h, bk_hdc, rc_tmp.x, rc_tmp.y, SRCCOPY);
  SetTextColor(hdc, MapRGB(hdc, 255, 255, 255));

  GetWindowText(hwnd, wbuf, 128); //��ð�ť�ؼ�������

  DrawText(hdc, wbuf, -1, &rc, DT_VCENTER|DT_CENTER);//��������(���ж��뷽ʽ)
}

/* �ػ�Բ����ʾ���� */
void Circle_Paint(HWND hwnd, HDC hdc)
{
  char  cbuf[128];
  WCHAR wbuf[128];
  RECT rc = {0, 0, CircleSize, CircleSize};

  EnableAntiAlias(hdc, TRUE);

  SetBrushColor(hdc, MapRGB(hdc, 65, 65, 65));
  FillArc(hdc, CircleSize/2, CircleSize/2, CircleCenter_2, CircleCenter_3, -45, 225);

  SetBrushColor(hdc, MapRGB(hdc, 200, 200, 200));
  FillArc(hdc, CircleSize/2, CircleSize/2, CircleCenter_2+1, CircleCenter_3-1, -45, ((225 - (-45))) * ADC_Vol / 3.3  - 45);

  EnableAntiAlias(hdc, FALSE);
  /* �������� */
  X_MeterPointer(hdc, CircleSize/2, CircleSize/2, CircleCenter_1, MapRGB(hdc,250,20,20), ADC_Vol);

  /* ʹ��Ĭ������ */
	SetFont(hdc, defaultFont);

  rc.w = 24*4;
  rc.h = 30;
  rc.x = CircleSize/2 - rc.w/2;
  rc.y = CircleSize/2 - rc.h/2;

  /* ��ʾ��ѹ�ٷֱ� */
  x_sprintf(cbuf, "%d%%", (int)(ADC_Vol/3.3*100));
  x_mbstowcs_cp936(wbuf, cbuf, 128);
  DrawText(hdc, wbuf, -1, &rc, DT_VCENTER|DT_CENTER);    // ��������(���ж��뷽ʽ)

  /* ��ʾ�ɼ����ĵ�ѹֵ */
  rc.y = CircleSize/2 - rc.h/2 + CircleCenter_3;
  x_sprintf(cbuf, "%.2fV", ADC_Vol);
  x_mbstowcs_cp936(wbuf, cbuf, 128);
  DrawText(hdc, wbuf, -1, &rc, DT_VCENTER|DT_CENTER);    // ��������(���ж��뷽ʽ)
}

/*
 * @brief  �ػ�������ʾ���ȵ�Բ
 * @param  HDC:	�Զ�����ƽṹ��
 * @retval NONE
*/
#define CircleCenter_r    25
#define CircleCenter_R    35
void BrighCircle_Paint(HDC hdc)
{
  /* �Ȼ���� */
  SetBrushColor(hdc, MapRGB(hdc, 200, 200, 200));
  EnableAntiAlias(hdc, TRUE);
  FillArc(hdc, GUI_XSIZE + 100, 105, 15, 18, 0, 360); 
  // EnableAntiAlias(hdc, FALSE);

  /* �ұ� */
  SetBrushColor(hdc, MapRGB(hdc, 255, 255, 255));
  // EnableAntiAlias(hdc, TRUE);
  FillArc(hdc, GUI_XSIZE + 700, 105, 16, 19, 0, 360); 
  EnableAntiAlias(hdc, FALSE);

  int Little_x, Little_y;
  int Big_x,    Big_y;

  SetPenColor(hdc, MapRGB(hdc, 255, 255, 255));

  for(int i=0; i<8; i++)
  {
    /* ����Բ�ĵ� */
    Little_x = GUI_XSIZE + 700 + CircleCenter_r * sin(0.785 * i);    // 0.785 = 3.14/4 = ��/4 = 45��
    Little_y = 105 - CircleCenter_r * cos(0.785 * i);

    /* Զ��Բ�ĵ� */
    Big_x = GUI_XSIZE + 700 + CircleCenter_R * sin(0.785 * i);    // 0.785 = 3.14/4 = ��/4 = 45��
    Big_y = 105 - CircleCenter_R * cos(0.785 * i);

    /* ��ֱ�� */
    Line(hdc, Little_x, Little_y, Big_x, Big_y);
  }
}

static LRESULT	ADCWinProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  switch(msg)
  {
    case WM_CREATE:
    {
      RECT rc;
      HWND hwnd_scrolbar;
      SCROLLINFO sif;/*���û������Ĳ���*/
      GetClientRect(hwnd, &rc);
      Rheostat_Init();    // ��ʼ�� ADC

      /*********************���ȵ��ڻ�����******************/
      sif.cbSize = sizeof(sif);
      sif.fMask = SIF_ALL;
      sif.nMin = 0;
      sif.nMax = 100;
      sif.nValue = 50;//��ʼֵ
      sif.TrackSize = 60;//����ֵ
      sif.ArrowSize = 0;//���˿��Ϊ0��ˮƽ��������          
      hwnd_scrolbar = CreateWindow(SCROLLBAR, L"SCROLLBAR_Brigh", WS_OWNERDRAW | WS_VISIBLE,//  
                      GUI_XSIZE + 100, (GUI_YSIZE - TitleHeight * 2) / 2, 600, 60, hwnd, SCROLLBAR_Brigh_ID, NULL, NULL);
      SendMessage(hwnd_scrolbar, SBM_SETSCROLLINFO, TRUE, (LPARAM)&sif); 

      rc.w = 24*5;
      rc.h = TitleHeight;
      rc.x = GUI_XSIZE + GUI_XSIZE / 2 - rc.w / 2;
      rc.y = TitleHeight;

      Brigh_Handle = CreateWindow(TEXTBOX, L"50%", WS_VISIBLE | WS_OWNERDRAW, rc.x, rc.y, rc.w, rc.h, hwnd, ID_TEXTBOX_Brigh, NULL, NULL);//
      SendMessage(Brigh_Handle, TBM_SET_TEXTFLAG, 0, DT_VCENTER | DT_CENTER | DT_BKGND);   

      SetTimer(hwnd, 2, 10, TMR_START, NULL);

      break;
    } 

    case WM_TIMER:
    {
      RECT rc;
      int tmr_id;
      
      tmr_id = wParam;
      GetWindowRect(hwnd, &rc);
      SetForegroundWindow(hwnd);

      if (tmr_id == MOVE_WIN)
      {
        if (AovingDirection == LeftToRight)
        {
          if (rc.x < 0)
          {
            OffsetRect(&rc, (rc.w >> 3), 0);
            rc.x = MIN(rc.x, 0);
            MoveWindow(hwnd, rc.x, rc.y, rc.w, rc.h, TRUE);
          }
          else
          {
            SetWindowText(Title_Handle,L"ADCһ��λ����ѹ��ʾ");
            KillTimer(hwnd, 1);
          }
        }
        else if (AovingDirection == RightToLeft)
        {
          if (rc.x > -800)
          {
            OffsetRect(&rc, -(rc.w >> 3), 0);
            rc.x = MAX(rc.x, -800);
            MoveWindow(hwnd, rc.x, rc.y, rc.w, rc.h, TRUE);
          }
          else
          {
            SetWindowText(Title_Handle,L"��Ļ���ȵ���");
            KillTimer(hwnd, 1);
          }
        }
      }
      else if (tmr_id == 2)
      {
        RECT rc;
        static double ADC_buff = 0.0;
        double vol_buff = 0.0;
        static uint8_t xC = 0;

        vol_buff =(double) ADC_ConvertedValue/4096*(double)3.3; // ��ȡת����ADֵ
//        GUI_DEBUG("��ѹֵǰΪ��%f", ADC_Vol);
        #if 1

          if (xC++ < 10)
          {
            ADC_buff += vol_buff;
            break;
          }
          else
          {
            ADC_Vol = ADC_buff / ((double)(xC-1));
            ADC_buff = 0;
            xC = 0;
          }

        #else
          
        ADC_Vol = (double)(((int)(vol_buff * 10)) / 10.0);

        #endif
//        GUI_DEBUG("��ѹֵ��Ϊ��%f", ADC_Vol);
        
        rc.x = Circle_X;
        rc.y = Circle_Y;
        rc.w = CircleSize * 2;
        rc.h = CircleSize * 2;

        InvalidateRect(hwnd, &rc, FALSE);
      }
      
      
      break;
    }

    case WM_ERASEBKGND:
    {
      
      HDC hdc =(HDC)wParam;
      RECT rc = {0, TitleHeight, GUI_XSIZE, GUI_YSIZE - TitleHeight};

      ScreenToClient(hwnd, (POINT *)&rc, 1);
      BitBlt(hdc, rc.x, rc.y, GUI_XSIZE, rc.h, bk_hdc, 0, TitleHeight, SRCCOPY);

      return TRUE;
    }

    case WM_PAINT:
    {
      HDC hdc, hdc_mem;
      PAINTSTRUCT ps;
      RECT rc = {Circle_X, Circle_Y, 2, 2};

      ClientToScreen(hwnd, (POINT *)&rc, 1);

      hdc_mem = CreateMemoryDC(SURF_SCREEN, CircleSize, CircleSize);

      hdc = BeginPaint(hwnd, &ps);

      BitBlt(hdc_mem, 0, 0, CircleSize, CircleSize, bk_hdc, rc.x, rc.y, SRCCOPY);

      Circle_Paint(hwnd, hdc_mem);
      BrighCircle_Paint(hdc);
      
      BitBlt(hdc, Circle_X, Circle_Y, CircleSize, CircleSize, hdc_mem, 0, 0, SRCCOPY);
      DeleteDC(hdc_mem);
      EndPaint(hwnd, &ps);

      break;
    } 

    case WM_NOTIFY:
    {
//      u16 code;
      u16 ctr_id;
      NMHDR *nr;
//      code=HIWORD(wParam);//��ȡ��Ϣ������
      ctr_id = LOWORD(wParam); //wParam��16λ�Ƿ��͸���Ϣ�Ŀؼ�ID.
      nr = (NMHDR*)lParam; //lParam����������NMHDR�ṹ�忪ͷ.      

      if (ctr_id == SCROLLBAR_Brigh_ID)
      {
        NM_SCROLLBAR *sb_nr;
        int i = 0;
        WCHAR wbuf[128];
        sb_nr = (NM_SCROLLBAR*)nr; //Scrollbar��֪ͨ��Ϣʵ��Ϊ NM_SCROLLBAR��չ�ṹ,���渽���˸������Ϣ.
        switch (nr->code)
        {
          case SBN_THUMBTRACK: //R�����ƶ�
          {
            i = sb_nr->nTrackValue; //��û��鵱ǰλ��ֵ                
            SendMessage(nr->hwndFrom, SBM_SETVALUE, TRUE, i); //���ý���ֵ
            x_wsprintf(wbuf, L"%d%%", i);
            SetWindowText(Brigh_Handle, wbuf);
          }
          break;
        }
      }   
      break;
    } 
 
    
    static int x_move;

    case WM_LBUTTONDOWN:
    {
      int x;
      x = LOWORD(lParam);
      RECT rc;
	  	GetWindowRect(hwnd, &rc);
      rc.x = x;
      ClientToScreen(hwnd, (POINT *)&rc, 1);
      x_move = rc.x;
//      GUI_DEBUG("���£�x = %d",x);
      break;
    }

    static int x_old;
    case WM_MOUSEMOVE:
    {
      int x;
      RECT rc;
      RECT Client_rc;
	  	GetWindowRect(hwnd, &rc);
      GetWindowRect(hwnd, &Client_rc);
      x = LOWORD(lParam);
      
      rc.x = x;
      ClientToScreen(hwnd, (POINT *)&rc, 1);
      OffsetRect(&Client_rc, rc.x - x_move, 0);

      Client_rc.x = MIN(Client_rc.x, 30);
      Client_rc.x = MAX(Client_rc.x, -800 - 30);
      
      MoveWindow(hwnd, Client_rc.x, Client_rc.y, Client_rc.w, Client_rc.h, TRUE);

      x_old = x_move;
      x_move = rc.x;

//      GUI_DEBUG("�ƶ���x = %d", x_old);
      break;
    }

    case WM_LBUTTONUP:
    { 
      int x;
      RECT Client_rc;
      RECT rc;
      
      x = LOWORD(lParam);
      rc.x = x;
      GetWindowRect(hwnd, &Client_rc);
      ClientToScreen(hwnd, (POINT *)&rc, 1);
//      GUI_DEBUG("̧��x = %d",rc.x);

      if (rc.x - x_old > 0)    // �������һ�
      {
        if (Client_rc.x > 0)
        {
          MoveWindow(hwnd, 0, Client_rc.y, Client_rc.w, Client_rc.h, TRUE);
        }
        else 
        {
          SetTimer(hwnd, 1, 5, TMR_START, NULL);
          AovingDirection = LeftToRight;
        }
      }
      else    // ��������
      {
        if (Client_rc.x < -800)
        {
          MoveWindow(hwnd, -800, Client_rc.y, Client_rc.w, Client_rc.h, TRUE);
        }
        else 
        {
          SetTimer(hwnd, 1, 5, TMR_START, NULL);
          AovingDirection = RightToLeft;
        }
      }
      
      break;
    } 

    case WM_DRAWITEM:    // ��ť�ػ�
    {
       DRAWITEM_HDR *ds;
       ds = (DRAWITEM_HDR*)lParam;
       switch(ds->ID)
       {
          case eID_ADC_EXIT:
          {
            CollectVoltage_ExitButton_OwnerDraw(ds);
            return TRUE;             
          }   

          case SCROLLBAR_Brigh_ID:
          {
            scrollbar_owner_draw(ds);
            return TRUE;             
          } 

          case ID_TEXTBOX_Brigh:
          {
            Textbox_OwnerDraw(ds);
            return TRUE;             
          } 
       }
       break;
    }

    case WM_DESTROY:
    {
      

      return PostQuitMessage(hwnd);	
    } 

    default:
      return	DefWindowProc(hwnd, msg, wParam, lParam);   
  }
  
  return WM_NULL;
}
 
static LRESULT	CollectVoltage_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  switch(msg)
  {
    case WM_CREATE:
    {
      RECT rc;
      GetClientRect(hwnd, &rc); 
            
      CreateWindow(BUTTON, L"O", WS_TRANSPARENT|BS_FLAT | BS_NOTIFY |WS_OWNERDRAW|WS_VISIBLE,
                  730, 0, 70, 70, hwnd, eID_ADC_EXIT, NULL, NULL); 

      WNDCLASS wcex;

      wcex.Tag	 		    = WNDCLASS_TAG;
      wcex.Style			  = CS_HREDRAW | CS_VREDRAW;
      wcex.lpfnWndProc	= (WNDPROC)ADCWinProc;
      wcex.cbClsExtra		= 0;
      wcex.cbWndExtra		= 0;
      wcex.hInstance		= NULL;
      wcex.hIcon			  = NULL;
      wcex.hCursor		  = NULL;
      
      rc.x = 0;
      rc.y = TitleHeight;
      rc.w = GUI_XSIZE*2;
      rc.h = GUI_YSIZE - TitleHeight * 2;
      ////����"ADC�ɼ�����"�Ŀؼ�.
      ADC_Handle = CreateWindowEx(WS_EX_NOFOCUS|WS_EX_FRAMEBUFFER ,&wcex,L"---",WS_CLIPCHILDREN|WS_VISIBLE,rc.x,rc.y,rc.w,rc.h,hwnd,ID_ADV_WIN,NULL,NULL);

      rc.w = GUI_XSIZE / 2;
      rc.h = TitleHeight;
      rc.x = GUI_XSIZE / 2 - rc.w / 2;
      rc.y = 0;

      Title_Handle = CreateWindow(TEXTBOX, L"ADCһ��λ����ѹ��ʾ", WS_VISIBLE | WS_OWNERDRAW, rc.x, rc.y, rc.w, rc.h, hwnd, ID_TEXTBOX_Title, NULL, NULL);//
      SendMessage(Title_Handle, TBM_SET_TEXTFLAG, 0, DT_VCENTER | DT_CENTER | DT_BKGND);   

      BOOL res;
      u8 *jpeg_buf;
      u32 jpeg_size;
      JPG_DEC *dec;
      res = RES_Load_Content(GUI_ADC_BACKGROUNG_PIC, (char**)&jpeg_buf, &jpeg_size);
      bk_hdc = CreateMemoryDC(SURF_SCREEN, GUI_XSIZE, GUI_YSIZE);
      if(res)
      {
        /* ����ͼƬ���ݴ���JPG_DEC��� */
        dec = JPG_Open(jpeg_buf, jpeg_size);

        /* �������ڴ���� */
        JPG_Draw(bk_hdc, 0, 0, dec);

        /* �ر�JPG_DEC��� */
        JPG_Close(dec);
      }
      /* �ͷ�ͼƬ���ݿռ� */
      RES_Release_Content((char **)&jpeg_buf);

    //  SetTimer(hwnd,2,0,TMR_SINGLE,NULL);

      break;
    } 
    case WM_TIMER:
    {
      
      
      break;
    }
    // case WM_ERASEBKGND:
    // {
      
    //   HDC hdc =(HDC)wParam;
    //   RECT rc =*(RECT*)lParam;
      
    //   SetBrushColor(hdc, MapRGB(hdc, 255, 0, 0));;
    //   FillRect(hdc, &rc);

    //   return TRUE;
    //   break;
    // }

    case WM_PAINT:
    {
      HDC hdc;
      PAINTSTRUCT ps;
      //  RECT rc = {0,0,800,70};
      //  hdc_mem = CreateMemoryDC(SURF_ARGB4444, 800,70);
       
      hdc = BeginPaint(hwnd, &ps);

      
       
      //  SetBrushColor(hdc_mem, MapARGB(hdc_mem,100,105, 105, 105));
      //  FillRect(hdc_mem, &rc);

      BitBlt(hdc, 0, 0, GUI_XSIZE, GUI_YSIZE, bk_hdc, 0, 0, SRCCOPY);
      
      //  BitBlt(hdc, 0,0,800,70,hdc_mem,0,0,SRCCOPY);
      //  DeleteDC(hdc_mem);
      EndPaint(hwnd, &ps);

      break;
    } 
    case WM_DRAWITEM:
    {
       DRAWITEM_HDR *ds;
       ds = (DRAWITEM_HDR*)lParam;
       switch(ds->ID)
       {
          case eID_ADC_EXIT:
          {
            CollectVoltage_ExitButton_OwnerDraw(ds);
            return TRUE;             
          }    

          case ID_TEXTBOX_Title:
          {
            Textbox_OwnerDraw(ds);
            return TRUE;             
          } 
       }

       break;
    }
    case WM_NOTIFY:
    {
      u16 code, id;
      id  =LOWORD(wParam);//��ȡ��Ϣ��ID��
      code=HIWORD(wParam);//��ȡ��Ϣ������    
      if(code == BN_CLICKED && id == eID_ADC_EXIT)
      {
        PostCloseMessage(hwnd);
        break;
      }

      break;
    } 

    case WM_LBUTTONUP:
    {
      return	DefWindowProc(hwnd, msg, wParam, lParam);   
    }

    case WM_MOUSEMOVE:
    {
      return	DefWindowProc(hwnd, msg, wParam, lParam);
    }

    case WM_LBUTTONDOWN:
    {
      return	DefWindowProc(hwnd, msg, wParam, lParam);   
    }

    case WM_DESTROY:
    {
      Rheostat_DISABLE();    // ֹͣADC�Ĳɼ�
      DeleteDC(bk_hdc);
      return PostQuitMessage(hwnd);	
    } 

    default:
      return	DefWindowProc(hwnd, msg, wParam, lParam);   
  }
  
  return WM_NULL;
  
}

void GUI_ADC_CollectVoltage_Dialog(void)
{
	
	WNDCLASS	wcex;
	MSG msg;

	wcex.Tag = WNDCLASS_TAG;

	wcex.Style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = CollectVoltage_proc; //������������Ϣ����Ļص�����.
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = NULL;//hInst;
	wcex.hIcon = NULL;//LoadIcon(hInstance, (LPCTSTR)IDI_WIN32_APP_TEST);
	wcex.hCursor = NULL;//LoadCursor(NULL, IDC_ARROW);
   
	//����������
	MAIN_Handle = CreateWindowEx(WS_EX_NOFOCUS|WS_EX_FRAMEBUFFER,
                              &wcex,
                              L"GUI_ADC_CollectVoltage_Dialog",
                              WS_VISIBLE|WS_CLIPCHILDREN,
                              0, 0, GUI_XSIZE, GUI_YSIZE,
                              NULL, NULL, NULL, NULL);
   //��ʾ������
	ShowWindow(MAIN_Handle, SW_SHOW);
	//��ʼ������Ϣѭ��(���ڹرղ�����ʱ,GetMessage������FALSE,�˳�����Ϣѭ��)��
	while (GetMessage(&msg, MAIN_Handle))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}  
}


