#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "B_Play.h"
#include "B_Public.h"
#define SDL2_0
/*************************************************
 *  接口名称�?B_Play_Openfile
 *  功能�?   打开媒体文件
 *  创建日期�?2021/07/25
 *  输入�?   const char* szFilePath:  媒体文件Path
 *  输出�?   �? *  返回�?   成功: 0
 * 			 失败:错误�?**************************************************/
 
 
 //���ļ��޸Ĳ���
 
 
int B_Play_Openfile(const char* szFilePath)
{
	int iRet = 0;
	int iVideo_index = 0;
	char szError[128] = {0};
	unsigned char *out_buffer = NULL;
	AVFormatContext *FormatContext = avformat_alloc_context();  	/* 媒体文件句柄 */
	AVCodec *Video_Codec = NULL;									/* 解码器句�?*/
	AVCodecContext *Video_CodecContext = NULL;   					/* 解码器上下文 */
	AVFrame *Video_Frame = NULL;									/* 帧缓�?*/
	AVFrame *YUV_Frame = NULL;										/* YUV�?*/									
	AVPacket *Video_Packet = NULL;
	int ret, got_picture;

	if (szFilePath == NULL)
	{
		B_LOG("illegal file path\r\n");
		return -1;
	}
	else
	{
		B_LOG(">>>>>>>>>>>>>>>>>>>>>>>>>>>B_Play_Openfile, Path[%s]\r\n", szFilePath);
	}

	/* 打开一个输入流并读取头�?*/
	iRet = avformat_open_input(&FormatContext, szFilePath, NULL, NULL);
	if (iRet != 0)
	{
		av_strerror(iRet, szError, sizeof(szError));
		B_LOG("avformat_open_input fail, Ret[%d], Err[%s]\r\n", iRet, szError);
		return iRet;
	}

	/* 读取媒体文件的数据包以获取流信息 */
	iRet = avformat_find_stream_info(FormatContext, NULL);
	if (iRet < 0)
	{
		av_strerror(iRet, szError, sizeof(szError));
		B_LOG("avformat_open_input fail, Ret[%d], Err[%s]\r\n", iRet, szError);
		avformat_close_input(&FormatContext);
		return iRet;
	}

	/* 查找视频码流索引 */
	iVideo_index = av_find_best_stream(FormatContext, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
	if (AVERROR_STREAM_NOT_FOUND == iVideo_index)
	{
		/* 未找到视频流 */
		B_LOG("av_find_best_stream not found video stream\r\n");
		avformat_close_input(&FormatContext);
		return -1;
	}
	else if (AVERROR_DECODER_NOT_FOUND == iVideo_index)
	{
		/* 找到视频流，但是没找到解码器 */
		B_LOG("av_find_best_stream streams were found but no decoder\r\n");
		avformat_close_input(&FormatContext);
		return -1;
	}

	/* 查找解码�?*/
	Video_Codec = avcodec_find_decoder(FormatContext->streams[iVideo_index]->codecpar->codec_id);
	if (NULL == Video_Codec)
	{
		B_LOG("avcodec_find_decoder fail\r\n");
		avformat_close_input(&FormatContext);
		return -1;
	}

	/* 创建解码器上下文(为Video_CodecContext申请内存空间) */
	Video_CodecContext = avcodec_alloc_context3(Video_Codec);
	if (NULL == Video_CodecContext)
	{
		B_LOG("avcodec_alloc_context3 fail\r\n");
		avformat_close_input(&FormatContext);
		return -1;
	}

	/* 预初始化解码器上下文(从FormatContext->streams[iVideo_index]->codecpar获取解码器的有关参数,并填充至Video_CodecContext) */
	iRet = avcodec_parameters_to_context(Video_CodecContext, FormatContext->streams[iVideo_index]->codecpar);
	if (iRet < 0)
	{
		av_strerror(iRet, szError, sizeof(szError));
		B_LOG("avcodec_parameters_to_context fail, Ret[%d], Err[%s]\r\n", iRet, szError);
		avformat_close_input(&FormatContext);
		return iRet;
	}

	/* 打开解码器上下文 */
	iRet = avcodec_open2(Video_CodecContext, Video_Codec, NULL);
	if (iRet != 0)
	{
		av_strerror(iRet, szError, sizeof(szError));
		B_LOG("avcodec_open2 fail, Ret[%d], Err[%s]\r\n", iRet, szError);
		avformat_close_input(&FormatContext);
		return iRet;
	}

	/* 申请帧缓�?*/
	Video_Frame = av_frame_alloc();
	if (NULL == Video_Frame)
	{
		B_LOG("av_frame_alloc fail,\r\n");
		avformat_close_input(&FormatContext);
		return iRet;
	}

	YUV_Frame = av_frame_alloc();
	if (NULL == YUV_Frame)
	{
		B_LOG("av_frame_alloc fail,\r\n");
		avformat_close_input(&FormatContext);
		return iRet;
	}
#ifdef SDL2_0
	int Video_H = 0;												/* 视频�?*/
	int Video_W = 0;

	out_buffer=(unsigned char *)av_malloc(av_image_get_buffer_size(AV_PIX_FMT_YUV420P,  Video_CodecContext->width, Video_CodecContext->height,1));
	if (NULL == out_buffer)
	{
		B_LOG("av_malloc fail,\r\n");
		avformat_close_input(&FormatContext);
		return iRet;
	}
	struct SwsContext *img_convert_ctx;

	av_image_fill_arrays(YUV_Frame->data, YUV_Frame->linesize,out_buffer, AV_PIX_FMT_YUV420P,Video_CodecContext->width, Video_CodecContext->height,1);
	Video_Packet = (AVPacket *)av_malloc(sizeof(AVPacket));
	av_dump_format(FormatContext, 0, szFilePath, 0);
	img_convert_ctx = sws_getContext(Video_CodecContext->width, Video_CodecContext->height, Video_CodecContext->pix_fmt, 
		Video_CodecContext->width, Video_CodecContext->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);
	iRet = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER);
	if (iRet != 0)
	{
		B_LOG("SDL_Init fail, Ret[%d]\r\n", iRet);
		avformat_close_input(&FormatContext);
		return iRet;
	}
	
	Video_W = Video_CodecContext->width;
	Video_H = Video_CodecContext->height;
	SDL_Window *screen = SDL_CreateWindow("B_Play", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, Video_W, Video_H, SDL_WINDOW_OPENGL);
	if(!screen) 
	{  
		B_LOG("SDL: could not create window - exiting:%s\n",SDL_GetError());  
		return -1;
	}

	SDL_Renderer *sdlRenderer = SDL_CreateRenderer(screen, -1, 0);  
	SDL_Texture *sdlTexture = SDL_CreateTexture(sdlRenderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING,Video_CodecContext->width,Video_CodecContext->height);  
	
	SDL_Rect sdlRect;
	sdlRect.x=0;
	sdlRect.y=0;
	sdlRect.w=Video_W;
	sdlRect.h=Video_H;

	while(av_read_frame(FormatContext, Video_Packet)>=0){
		if(Video_Packet->stream_index==iVideo_index){
			//iRet = avcodec_decode_video2(Video_CodecContext, Video_Frame, &got_picture, Video_Packet);
			avcodec_send_packet(Video_CodecContext, Video_Packet);
			got_picture = avcodec_receive_frame(Video_CodecContext, Video_Frame);
			if(!got_picture){
				sws_scale(img_convert_ctx, (const unsigned char* const*)Video_Frame->data, Video_Frame->linesize, 0, Video_CodecContext->height, YUV_Frame->data, YUV_Frame->linesize);
				SDL_UpdateYUVTexture(sdlTexture, &sdlRect,
				YUV_Frame->data[0], YUV_Frame->linesize[0],
				YUV_Frame->data[1], YUV_Frame->linesize[1],
				YUV_Frame->data[2], YUV_Frame->linesize[2]);
				
				SDL_RenderClear( sdlRenderer );  
				SDL_RenderCopy( sdlRenderer, sdlTexture,  NULL, &sdlRect);  
				SDL_RenderPresent( sdlRenderer );  
				
				SDL_Delay(40);
			}
		}
		av_packet_unref(Video_Packet);
	}

	while (1) {
		//ret = avcodec_decode_video2(Video_CodecContext, Video_Frame, &got_picture, Video_Packet);
		avcodec_send_packet(Video_CodecContext, Video_Packet);
		got_picture = avcodec_receive_frame(Video_CodecContext, Video_Frame);
		if (!got_picture)
			break;
		sws_scale(img_convert_ctx, (const unsigned char* const*)Video_Frame->data, Video_Frame->linesize, 0, Video_CodecContext->height, 
			YUV_Frame->data, YUV_Frame->linesize);

		SDL_UpdateTexture( sdlTexture, &sdlRect, YUV_Frame->data[0], YUV_Frame->linesize[0] );  
		SDL_RenderClear( sdlRenderer );  
		SDL_RenderCopy( sdlRenderer, sdlTexture,  NULL, &sdlRect);  
		SDL_RenderPresent( sdlRenderer );  

		SDL_Delay(40);
	}
 
	sws_freeContext(img_convert_ctx);
	SDL_Quit();
	av_frame_free(&YUV_Frame);
	av_frame_free(&Video_Frame);
	avcodec_close(Video_CodecContext);
	avformat_close_input(&FormatContext);
	
#else
	struct SwsContext *img_convert_ctx;
	int Video_H = 0;												/* 视频�?*/
	int Video_W = 0;												/* 视频�?*/
	SDL_Surface *screen; 
	SDL_VideoInfo *vi;
	SDL_Overlay *bmp; 
	SDL_Rect rect;
	int got_picture;
	iRet = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER);
	if (iRet != 0)
	{
		B_LOG("SDL_Init fail, Ret[%d]\r\n", iRet);
		avformat_close_input(&FormatContext);
		return iRet;
	}

	Video_H = Video_CodecContext->width;
	Video_W = Video_CodecContext->height;
	screen = SDL_SetVideoMode(Video_W, Video_H, 0, 0);
	if (screen == NULL)
	{
		B_LOG("SDL_SetVideoMode fail, Ret[%d]\r\n", iRet);
		avformat_close_input(&FormatContext);
		return iRet;
	}
	bmp = SDL_CreateYUVOverlay(Video_CodecContext->width, Video_CodecContext->height,SDL_YV12_OVERLAY, screen); 
 
	rect.x = 0;    
	rect.y = 0;    
	rect.w = Video_W;    
	rect.h = Video_H; 

	Video_Packet = (AVPacket *)av_malloc(sizeof(AVPacket));
	av_dump_format(FormatContext, 0, szFilePath, 0);


	SDL_WM_SetCaption("B_Play",NULL);

	img_convert_ctx = sws_getContext(Video_CodecContext->width, Video_CodecContext->height, Video_CodecContext->pix_fmt, 
			Video_CodecContext->width, Video_CodecContext->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL); 
	B_LOG("1111111");
	while(av_read_frame(FormatContext, Video_Packet)>=0)
	{
		B_LOG("2222222");
		if(Video_Packet->stream_index==iVideo_index)
		{
			//Decode
			avcodec_send_packet(Video_CodecContext, Video_Packet);
			got_picture = avcodec_receive_frame(Video_CodecContext, Video_Frame);

			if(!got_picture)
			{
				SDL_LockYUVOverlay(bmp);
				YUV_Frame->data[0]=bmp->pixels[0];
				YUV_Frame->data[1]=bmp->pixels[2];
				YUV_Frame->data[2]=bmp->pixels[1];     
				YUV_Frame->linesize[0]=bmp->pitches[0];
				YUV_Frame->linesize[1]=bmp->pitches[2];   
				YUV_Frame->linesize[2]=bmp->pitches[1];
				sws_scale(img_convert_ctx, (const uint8_t* const*)Video_Frame->data, Video_Frame->linesize, 0, 
					Video_CodecContext->height, YUV_Frame->data, YUV_Frame->linesize);
				SDL_UnlockYUVOverlay(bmp); 

				SDL_DisplayYUVOverlay(bmp, &rect); 
				//Delay 40ms
				SDL_Delay(40);
			}
		}

		av_packet_unref(Video_Packet);
	}

	B_LOG("3333333333\n");
	while (1) 
	{
		B_LOG("4444444444\n");
		avcodec_send_packet(Video_CodecContext, Video_Packet);

		got_picture = avcodec_receive_frame(Video_CodecContext, Video_Frame);
		if (got_picture)
			break;
		sws_scale(img_convert_ctx, (const uint8_t* const*)Video_Frame->data, Video_Frame->linesize, 0, Video_CodecContext->height, YUV_Frame->data, YUV_Frame->linesize);
		SDL_LockYUVOverlay(bmp);
		YUV_Frame->data[0]=bmp->pixels[0];
		YUV_Frame->data[1]=bmp->pixels[2];
		YUV_Frame->data[2]=bmp->pixels[1];     
		YUV_Frame->linesize[0]=bmp->pitches[0];
		YUV_Frame->linesize[1]=bmp->pitches[2];   
		YUV_Frame->linesize[2]=bmp->pitches[1];
 
		SDL_UnlockYUVOverlay(bmp); 
		SDL_DisplayYUVOverlay(bmp, &rect); 
		//Delay 40ms
		SDL_Delay(40);
	}
 
	sws_freeContext(img_convert_ctx);
	SDL_Quit();
	//av_free(out_buffer);
	av_free(YUV_Frame);
	avcodec_close(Video_CodecContext);
	avformat_close_input(&FormatContext);
#endif
	return 0;
}
