/*
	Water Rendering demo for the Nintendo 3DS by Gek
*/

#include <3ds.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <dirent.h>
//This include a header containing definitions of our image
#include "brew2_bgr.h"
#define CHAD_API_IMPL
#include "api.h"
#define WIDTH 320
#define HEIGHT 240
#define SCREENBYTES (320*240*3)
#define MAX(x,y) (x>y?x:y)
#define MIN(x,y) (x<y?x:y)

#define SAMPLERATE 22050
#define SAMPLESPERBUF (SAMPLERATE / 30)
#define BYTESPERSAMPLE 4
u8* ifb,* fb;
sprite heighttex, colortex;

struct{
	vec3 p;
	float ang;
	float horizon;
	float scale_height;
	float distance;
} camera;

void DrawVerticalLine(u32 x, u32 height, u32 low, u32 col){
	if(x >= WIDTH || height >= HEIGHT || low >= HEIGHT || low <= height) return;
	x %= WIDTH;
	//low = HEIGHT-1;
	//printf("\nGot color");
	height = HEIGHT - height;
	low = HEIGHT - low;
	for(u32 i = low; i <= height && i < HEIGHT; i++){
		//printf("\nwriting to %d, %d", x,i);
		((u32*)fb)[i + x * HEIGHT] = col;
	}
	//printf("\noh shit it actually worked");
}

ivec3 ftos(ivec3 samppoint){
	while (samppoint.d[0] < 0) samppoint.d[0] += heighttex.w;
	while (samppoint.d[1] < 0) samppoint.d[1] += heighttex.h;
	samppoint.d[0] %= heighttex.w;
	samppoint.d[1] %= heighttex.h;
	return samppoint;
}

u32 sampleheight(ivec3 s){
	/*
	if(s.d[0] < 0 || s.d[0] > 1024 ||
	   s.d[1] < 0 || s.d[1] > 1024)
	   return 0;
	*/
	s = ftos(s);
	uchar result = ((uchar*)heighttex.d)[
   		(
   	 		(s.d[0]) + 
   	 		(s.d[1]) * heighttex.w
   	 	)*4
   	];
   	return (u32)result;
}


inline u32 pxReverse(u32 x)
{

    return
    // Source is in format: 0xAARRGGBB
        ((x & 0xFF000000) >> 24) | //______AA
        ((x & 0x00FF0000) >>  8) | //____RR__
        ((x & 0x0000FF00) <<  8) | //__GG____
        ((x & 0x000000FF) << 24);  //BB______
    // Return value is in format:  0xBBGGRRAA
}

void swap_colors(){
	u32* datum = (u32*)colortex.d;
	for(int i = 0; i < colortex.w * colortex.h; i++)
		datum[i]= pxReverse(datum[i]);
}

u32 samplecolor(ivec3 s){
	s = ftos(s);
	u32 result =((u32*)colortex.d)
	[s.d[0] + s.d[1] * colortex.w];
	return result;
}


void Render(){
	vec3 p; float phi; float height; float horizon; float scale_height; float distance;
	//For this demo.
	p = camera.p;
	phi = camera.ang;
	height = camera.p.d[2];
	horizon = camera.horizon;
	scale_height = camera.scale_height;
	distance = camera.distance;
    //# precalculate viewing angle parameters
    float sinphi = sinf(phi);
    float cosphi = cosf(phi);
    
    //# initialize visibility array. Y position for each column on screen 
    float ybuffer[WIDTH];
    //for i in range(0, screen_width):
    for(int i = 0; i < WIDTH; i++)
        ybuffer[i] = HEIGHT;

    //# Draw from front to the back (low z coordinate to high z coordinate)
    float dz = 1.0;
    float z = 0.1;
    while(z < distance){
        //# Find line on map. This calculation corresponds to a field of view of 90°
        vec3 pleft = {
            .d[0] = (-cosphi*z - sinphi*z) + p.d[0],
            
            .d[1] = (sinphi*z - cosphi*z) + p.d[1],
            .d[2] = 0
            };
        vec3 pright = {
            .d[0] = ( cosphi*z - sinphi*z) + p.d[0],
            .d[1] = (-sinphi*z - cosphi*z) + p.d[1],
            .d[2] = 0
        };

        //# segment the line
        float dx = (pright.d[0] - pleft.d[0]) / WIDTH;
        float dy = (pright.d[1] - pleft.d[1]) / WIDTH;

        //# Raster line and draw a vertical line for each segment
        //for i in range(0, screen_width):
        for(int i = 0; i < WIDTH; i++){
        	ivec3 plefti;
        	plefti.d[0] = pleft.d[0];
        	plefti.d[1] = pleft.d[1];
			int hsamp = sampleheight(plefti);
            int height_on_screen = (
            	height - hsamp
            )/z * scale_height + horizon;
            u32 col = samplecolor(plefti);
			//printf("\nBefore line render, x = %d, z = %f", i, z);
            DrawVerticalLine(i, height_on_screen, ybuffer[i], col);
			//printf("\nAfter line render, x = %d, z = %f", i, z);
            if(height_on_screen < ybuffer[i])
                ybuffer[i] = height_on_screen;
            pleft.d[0] += dx;
            pleft.d[1] += dy;
		}
        //# Go to next line and increase step size when you are far away
        z += dz;
        dz += 0.05;
    }
}

//----------------------------------------------------------------------------
void fill_buffer(void *audioBuffer,size_t offset, size_t size, int frequency ) {
//----------------------------------------------------------------------------

	u32 *dest = (u32*)audioBuffer;
	for (int i=0; i<size; i++) {
		s16 sample = INT16_MAX * sin(frequency*(2*M_PI)*(offset+i)/SAMPLERATE);
		dest[i] = (sample<<16) | (sample & 0xffff);
	}
	DSP_FlushDataCache(audioBuffer,size);

}

int myscandir(){
	{ 
	    struct dirent *de;  // Pointer for directory entry 
	  
	    // opendir() returns a pointer of DIR type.  
	    DIR *dr = opendir("romfs:/"); 
	  
	    if (dr == NULL)  // opendir returns NULL if couldn't open directory 
	    { 
	        printf("Could not open current directory" ); 
	        return 0; 
	    } 
	  
	    // Refer http://pubs.opengroup.org/onlinepubs/7990989775/xsh/readdir.html 
	    // for readdir() 
	    while ((de = readdir(dr)) != NULL) 
	            printf("%s\n", de->d_name); 
	  
	    closedir(dr);
	    return 0 ;
	} 
}

int main(int argc, char **argv)
{
	uint RR_=3;
	uint GG_=2;
	uint BB_=1;
	uint AA_=0;
	track* mytrack = NULL;
	camera.p = (vec3){.d[0] = 1024,.d[1] = -100,.d[2] = 200};
		camera. ang = 3.14159 * 0.5; 
		camera. horizon = HEIGHT/2;
		camera. scale_height = 50;
		camera. distance = 500;
	//gfxInitDefault();
	gfxInit(GSP_RGBA8_OES,GSP_RGBA8_OES,false);
	fsInit();
	romfsInit();
	init();ainit();
	consoleInit(GFX_TOP, NULL);
	myscandir();
	//mytrack = lmus("romfs:/WWGW.wav");
	mytrack = lmus("romfs:/Strongest.mp3");
	lspr(&heighttex, "romfs:/D5W.png");
	lspr(&colortex, "romfs:/C5W.png");
	swap_colors();
		if(!mytrack){
			printf("\nError loading sounds\n");
		}
	if(mytrack)
		mplay(mytrack, -1, 1000);
	//Initialize console on top screen. Using NULL as the second argument tells the console library to use the internal console structure as current one
	
	//initFluid();
	printf("\nHeightmaps!\nThanks MR!");

	printf("\n\n<Start>: Exit program.\n");
	fb = gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, NULL, NULL);
	gfxSetDoubleBuffering(GFX_BOTTOM, true);
	
	

	//Get the bottom screen's frame buffer
	
	ifb = linearAlloc(320 * 240 * 4); //Enough for the bottom screen.
	if(!ifb) return 1;
	//Copy our image in the bottom screen's frame buffer
	//memcpy(fb, brew2_bgr, brew2_bgr_size);

	// Main loop
	u32 ox = 0;
	while (aptMainLoop())
	{
		camera.ang+=0.01;
		if(camera.ang > 2 * 3.14159)
			camera.ang -= 2*3.14159;
		camera.p.d[0]+= 2;
		if(camera.p.d[0] > 1024)
			camera.p.d[0]-=1024;
		//Scan all the inputs. This should be done once for each frame
		hidScanInput();

		//hidKeysDown returns information about which buttons have been just pressed (and they weren't in the previous frame)
		u32 kDown = hidKeysDown();
		u32 kHeld = hidKeysHeld();
		if (kDown & KEY_START) break; // break in order to return to hbmenu
		fb = gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, NULL, NULL);
		
		touchPosition touch;
		//Read the touch screen coordinates
		
		if (kHeld & KEY_TOUCH)
		{
			hidTouchRead(&touch);	
			
			touch.py = HEIGHT - touch.py;
			printf("Detected Touch!\n");
		}
		//Copy our image in the bottom screen's frame buffer
		//memcpy(fb, ifb, 320*240*3);
		//memcpy(ifb, brew2_bgr, brew2_bgr_size);
		fb = gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, NULL, NULL);
		for(int i = 0; i < 320 * 240 * 4; i++)
			if(i%4 == 1)
				fb[i] = 235;
			else if(i%4 == 2)
				fb[i] = 206;
			else if(i%4 == 3)
				fb[i] = 135;
		Render();
		
		
		//memcpy(fb, ifb, 320*240*4);
		// Flush and swap framebuffers
		gfxFlushBuffers();
		gfxSwapBuffers();
		
		//Wait for VBlank
		gspWaitForVBlank();
		
	}
	if(ifb) linearFree(ifb);
	
	// Exit services
	
	Mix_Quit();
	SDL_Quit();
	gfxExit();
	return 0;
}
