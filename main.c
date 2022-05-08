#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "B_Play.h"
#include "B_Public.h"

//0508修改测试

/***********测试*****************/

//Full Screen
#define SHOW_FULLSCREEN 0
//Output YUV420P 
#define OUTPUT_YUV420P 0
 

//#define MYDAEMON
int mydaemon();
/***********测试*****************/


/*************************************************
 *  接口名称�?main
 *  功能�?   B_Play进程入口�? *  创建日期�?2021/07/25
 *  输入�?   �? *  输出�?   �? *  返回�?   成功: 0
 * 			 失败:错误�?**************************************************/
int main(void)
{
	int iRet = 0;
	char szInput[64]="/home/book/Desktop/tt.mp4";
//mydaemon();
	iRet = B_Play_Openfile(szInput);
	if (iRet != 0)
	{
		B_LOG("B_Play_Openfile File, Ret[%d]\n", iRet);
		return iRet;
	}

	return 0;
}

#ifdef MYDAEMON
int mydaemon()
{
//FFmpeg
	AVFormatContext	*pFormatCtx;
	int				i, videoindex;
	AVCodecContext	*pCodecCtx;
	AVCodec			*pCodec;
	AVFrame	*pFrame,*pFrameYUV;
	AVPacket *packet;
	struct SwsContext *img_convert_ctx;
	//SDL
	int screen_w,screen_h;
	SDL_Surface *screen; 
	SDL_VideoInfo *vi;
	SDL_Overlay *bmp; 
	SDL_Rect rect;
 
	FILE *fp_yuv;
	int ret, got_picture;
	char filepath[]="/home/book/Desktop/tt.mp4";
 
	//av_register_all();
	avformat_network_init();
	
	pFormatCtx = avformat_alloc_context();
 
	if(avformat_open_input(&pFormatCtx,filepath,NULL,NULL)!=0){
		printf("Couldn't open input stream.\n");
		return -1;
	}
	if(avformat_find_stream_info(pFormatCtx,NULL)<0){
		printf("Couldn't find stream information.\n");
		return -1;
	}
	videoindex=-1;
	videoindex = av_find_best_stream(pFormatCtx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
	if (AVERROR_STREAM_NOT_FOUND == videoindex)
	{
		/* 未找到视频流 */
		B_LOG("av_find_best_stream not found video stream\r\n");
		avformat_close_input(&pFormatCtx);
		return -1;
	}
	
	pCodecCtx = avcodec_alloc_context3(pCodec);
	if (NULL == pCodecCtx)
	{
		B_LOG("avcodec_alloc_context3 fail\r\n");
		avformat_close_input(&pFormatCtx);
		return -1;
	}
pCodec = avcodec_find_decoder(pFormatCtx->streams[videoindex]->codecpar->codec_id);
	//pCodec=avcodec_find_decoder(pCodecCtx->codec_id);
	if(pCodec==NULL){
		printf("Codec not found.\n");
		return -1;
	}
	if(avcodec_open2(pCodecCtx, pCodec,NULL)<0){
		printf("Could not open codec.\n");
		return -1;
	}
	
	pFrame=av_frame_alloc();
	pFrameYUV=av_frame_alloc();
	//uint8_t *out_buffer=(uint8_t *)av_malloc(avpicture_get_size(PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height));
	//avpicture_fill((AVPicture *)pFrameYUV, out_buffer, PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height);
	//SDL----------------------------
	if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {  
		printf( "Could not initialize SDL - %s\n", SDL_GetError()); 
		return -1;
	} 
 
#if SHOW_FULLSCREEN
	vi = SDL_GetVideoInfo();
	screen_w = vi->current_w;
	screen_h = vi->current_h;
	screen = SDL_SetVideoMode(screen_w, screen_h, 0,SDL_FULLSCREEN);
#else
	screen_w = pCodecCtx->width;
	screen_h = pCodecCtx->height;
	screen = SDL_SetVideoMode(screen_w, screen_h, 0,0);
#endif
 
	if(!screen) {  
		printf("SDL: could not set video mode - exiting:%s\n",SDL_GetError());  
		return -1;
	}
 
	bmp = SDL_CreateYUVOverlay(pCodecCtx->width, pCodecCtx->height,SDL_YV12_OVERLAY, screen); 
 
	rect.x = 0;    
	rect.y = 0;    
	rect.w = screen_w;    
	rect.h = screen_h;  
	//SDL End------------------------
 
 
	packet=(AVPacket *)av_malloc(sizeof(AVPacket));
	//Output Information-----------------------------
	printf("------------- File Information ------------------\n");
	av_dump_format(pFormatCtx,0,filepath,0);
	printf("-------------------------------------------------\n");
 
#if OUTPUT_YUV420P 
    fp_yuv=fopen("output.yuv","wb+");  
#endif  
 
	SDL_WM_SetCaption("Simplest FFmpeg Player",NULL);
 
	img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL); 
	//------------------------------
	while(av_read_frame(pFormatCtx, packet)>=0){
		if(packet->stream_index==videoindex){
			//Decode
			

avcodec_send_packet(pCodecCtx, packet);

	got_picture = avcodec_receive_frame(pCodecCtx, pFrame);


			if(got_picture){
				SDL_LockYUVOverlay(bmp);
				pFrameYUV->data[0]=bmp->pixels[0];
				pFrameYUV->data[1]=bmp->pixels[2];
				pFrameYUV->data[2]=bmp->pixels[1];     
				pFrameYUV->linesize[0]=bmp->pitches[0];
				pFrameYUV->linesize[1]=bmp->pitches[2];   
				pFrameYUV->linesize[2]=bmp->pitches[1];
				sws_scale(img_convert_ctx, (const uint8_t* const*)pFrame->data, pFrame->linesize, 0, 
					pCodecCtx->height, pFrameYUV->data, pFrameYUV->linesize);
#if OUTPUT_YUV420P
				int y_size=pCodecCtx->width*pCodecCtx->height;  
				fwrite(pFrameYUV->data[0],1,y_size,fp_yuv);    //Y 
				fwrite(pFrameYUV->data[1],1,y_size/4,fp_yuv);  //U
				fwrite(pFrameYUV->data[2],1,y_size/4,fp_yuv);  //V
#endif
				
				SDL_UnlockYUVOverlay(bmp); 
 
				SDL_DisplayYUVOverlay(bmp, &rect); 
				//Delay 40ms
				SDL_Delay(40);
			}
		}
		av_packet_unref(packet);
	}
 
	//FIX: Flush Frames remained in Codec
	while (1) {
		

			avcodec_send_packet(pCodecCtx, packet);

	got_picture = avcodec_receive_frame(pCodecCtx, pFrame);
	if (got_picture)
			break;
		sws_scale(img_convert_ctx, (const uint8_t* const*)pFrame->data, pFrame->linesize, 0, pCodecCtx->height, pFrameYUV->data, pFrameYUV->linesize);
		
		SDL_LockYUVOverlay(bmp);
		pFrameYUV->data[0]=bmp->pixels[0];
		pFrameYUV->data[1]=bmp->pixels[2];
		pFrameYUV->data[2]=bmp->pixels[1];     
		pFrameYUV->linesize[0]=bmp->pitches[0];
		pFrameYUV->linesize[1]=bmp->pitches[2];   
		pFrameYUV->linesize[2]=bmp->pitches[1];
#if OUTPUT_YUV420P
		int y_size=pCodecCtx->width*pCodecCtx->height;  
		fwrite(pFrameYUV->data[0],1,y_size,fp_yuv);    //Y 
		fwrite(pFrameYUV->data[1],1,y_size/4,fp_yuv);  //U
		fwrite(pFrameYUV->data[2],1,y_size/4,fp_yuv);  //V
#endif
 
		SDL_UnlockYUVOverlay(bmp); 
		SDL_DisplayYUVOverlay(bmp, &rect); 
		//Delay 40ms
		SDL_Delay(40);
	}
 
	sws_freeContext(img_convert_ctx);
 
#if OUTPUT_YUV420P 
    fclose(fp_yuv);
#endif 
 
	SDL_Quit();
 
	//av_free(out_buffer);
	av_free(pFrameYUV);
	avcodec_close(pCodecCtx);
	avformat_close_input(&pFormatCtx);
 
	return 0;

	return;
}
#endif

