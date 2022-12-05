#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define res        1                        
#define SW         400*res                  //screen width
#define SH         240*res                  //screen height
#define SW2        (SW/2)                   //half of screen width
#define SH2        (SH/2)                   //half of screen height
#define depth      2
#define numSect 10
#define numWall 52


typedef struct 
{
 int fr1,fr2;           //frame 1 frame 2, to create constant frame rate
}frametime; frametime T;

typedef struct 
{
 int w,s,a,d;           //move up, down, left, right
 int sl,sr;             //strafe left, right 
 int m;                 //move up, down, look up, down
}keys; keys K;

typedef struct
{
	float cos[360];
	float sin[360];
}math; math M;

typedef struct
{
	int x,y,z;
	int a;
	int l;
}player; player P;

//--Walls--//
typedef struct
{
	int x1,y1; //bottom line point 1
	int x2,y2; //bottom line point 2
	int c;     //wall color
}walls; walls W[100];
//---------//

//--Sectors--//
typedef struct
{
	int ws,we;    //wall number start and end
	int z1,z2;    //height of bottom and top
	int d;        //add y distances to sort drawing order
	int c1,c2;
	int surf[SW];
	int surface;
}sectors; sectors S[100];
//-----------//

void pixel(int x, int y, int c) //bgr
{
	int r,g,b;
	if(c==0){ r=255; g=255; b=0;} //Yellow	
    if(c==1){ r=160; g=160; b=0;} //Yellow darker	
    if(c==2){ r=0; g=255; b=0;} //Green	
    if(c==3){ r=0; g=160; b=0;} //Green darker	
    if(c==4){ r=0; g=255; b=255;} //Cyan	
    if(c==5){ r=0; g=160; b=160;} //Cyan darker
    if(c==6){ r=160; g=100; b=0;} //brown	
    if(c==7){ r=110; g=50; b=0;} //brown darker
    if(c==8){ r=0; g=60; b=130;} //background 
	u8* fb = gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL);
	fb[((x*240)+y)*3+0] = b;
    fb[((x*240)+y)*3+1] = g;
    fb[((x*240)+y)*3+2] = r;
}

void pixelr(int x, int y, int c) //bgr
{
	int r,g,b;
	if(c==0){ r=255; g=255; b=0;} //Yellow	
    if(c==1){ r=160; g=160; b=0;} //Yellow darker	
    if(c==2){ r=0; g=255; b=0;} //Green	
    if(c==3){ r=0; g=160; b=0;} //Green darker	
    if(c==4){ r=0; g=255; b=255;} //Cyan	
    if(c==5){ r=0; g=160; b=160;} //Cyan darker
    if(c==6){ r=160; g=100; b=0;} //brown	
    if(c==7){ r=110; g=50; b=0;} //brown darker
    if(c==8){ r=0; g=60; b=130;} //background 
	u8* fbb = gfxGetFramebuffer(GFX_TOP, GFX_RIGHT, NULL, NULL);
	fbb[((x*240)+y)*3+0] = b;
    fbb[((x*240)+y)*3+1] = g;
    fbb[((x*240)+y)*3+2] = r;
}


void movePlayer()
{
	hidScanInput();
	u32 keysPressed = hidKeysDown();
	u32 kDown = hidKeysDown();
	u32 kHeld = hidKeysHeld();
	u32 kUp = hidKeysUp();
		
	//move left, right, up, down
	if (kDown & KEY_CPAD_LEFT || kHeld & KEY_CPAD_LEFT){P.a-=4; if(P.a<  0){P.a+=360;}}
	if (kDown & KEY_CPAD_RIGHT || kHeld & KEY_CPAD_RIGHT){P.a+=4; if(P.a>359){P.a-=360;}}
	
	int dx = M.sin[P.a]*10.0;
	int dy = M.cos[P.a]*10.0;
	
	if (kDown & KEY_X || kHeld & KEY_X){P.x+=dx; P.y+=dy;}
	if (kDown & KEY_B || kHeld & KEY_B){P.x-=dx; P.y-=dy;}
	
	//strafe left, right
	if (kDown & KEY_Y || kHeld & KEY_Y){P.x-=dy; P.y+=dx;}
	if (kDown & KEY_A || kHeld & KEY_A){P.x+=dy; P.y-=dx;}
	
	//move up, down, look up, look down
	if (kDown & KEY_CPAD_UP || kHeld & KEY_CPAD_UP){P.l-=1; }
	if (kDown & KEY_CPAD_DOWN || kHeld & KEY_CPAD_DOWN){P.l+=1; }
	if (kDown & KEY_DDOWN || kHeld & KEY_DDOWN){P.z-=4; }
	if (kDown & KEY_DUP || kHeld & KEY_DUP){P.z+=4; }

}

void clearBackground() 
{int x,y;
 for(y=0;y<SH;y++)
 { 
  for(x=0;x<SW;x++){ pixel(x,y,8);} //clear background color
 }	
}

void clearBackgroundr() 
{int x,y;
 for(y=0;y<SH;y++)
 { 
  for(x=0;x<SW;x++){ pixelr(x,y,8);} //clear background color
 }	
}

//--Clip Behind Player--//
void clipBehindPlayer(int *x1,int *y1, int *z1, int x2, int y2, int z2) //clip line
{
	float da=*y1;
	float db= y2;
	float d=da-db; if(d==0){d=1;}
	float s = da/(da-db);
	*x1 = *x1 + s* (x2-(*x1));
	*y1 = *y1 + s* (y2-(*y1)); if(*y1==0){ *y1=1;} //prevent divide by zero
	*z1 = *z1 + s* (z2-(*z1));
}
//----------------------//

void drawWall(int x1, int x2, int b1, int b2, int t1, int t2, int c, int s)
{int x,y;
    int dyb = b2-b1;
	int dyt = t2-t1;
	int dx  = x2-x1; if(dx==0){dx=1;}
	int xs=x1;
	
	if(x1<   1){x1=   1;}
	if(x2<   1){x2=   1;}
	if(x1>SW-1){x1=SW-1;}
	if(x2>SW-1){x2=SW-1;}
	
	for(x=x1;x<x2;x++)
	{
		int y1 = dyb*(x-xs+0.5)/dx+b1;
		int y2 = dyt*(x-xs+0.5)/dx+t1;
		
		if(y1<   1){y1=   1;}
		if(y2<   1){y2=   1;}
		if(y1>SH-1){y1=SH-1;}
		if(y2>SH-1){y2=SH-1;}
		
		float slider = osGet3DSliderState();
		float iod = slider/3;
		
		//surface
		if(S[s].surface== 1){ S[s].surf[x]=y1; continue;} //save bottom points
		if(S[s].surface== 2){ S[s].surf[x]=y2; continue;} //save top points
		if(S[s].surface==-1){ for(y=S[s].surf[x];y<y1;y++){pixel(x,y,S[s].c1);if (iod > 0.0f){pixelr(x-depth,y,S[s].c1);}};} //bottom surface
		if(S[s].surface==-2){ for(y=y2;y<S[s].surf[x];y++){pixel(x,y,S[s].c2);if (iod > 0.0f){pixelr(x-depth,y,S[s].c2);}};} //top surface
    	for(y=y1;y<y2;y++){pixel(x,y,c);if (iod > 0.0f){pixelr(x-depth,y,c);};}//normal wall
	}
}

//--Distance--//
int dist(int x1,int y1, int x2,int y2)
{
	int distance = sqrt( (x2-x1)*(x2-x1) + (y2-y1)*(y2-y1) );
	return distance;
}
//------------//

void draw3D()
{int s,w,loop, wx[4],wy[4],wz[4]; float CS=M.cos[P.a], SN=M.sin[P.a];
    
	//order sectors by distance
	for(s=0;s<numSect-1;s++)
	{
		for(w=0;w<numSect-s-1;w++)
		{
			if(S[w].d<S[w+1].d)
			{
			    sectors st=S[w]; S[w]=S[w+1]; S[w+1]=st;
			}
		}
	}
	
	//Draw sectors
	for(s=0;s<numSect;s++)
	{
		S[s].d=0;//clear distance
		     if(P.z<S[s].z1){ S[s].surface=1;}  //bottom surface
		else if(P.z>S[s].z2){ S[s].surface=2;}  //bottom surface
		else                { S[s].surface=0;}  //no surface
		
		for(loop=0;loop<2;loop++)
		{
		    for(w=S[s].ws;w<S[s].we;w++)
	        {
	            //Offset bottom 2 points by player
	            int x1 = W[w].x1-P.x, y1= W[w].y1-P.y;
	            int x2 = W[w].x2-P.x, y2= W[w].y2-P.y;
				
				//swap for surface
		    	if(loop==0){int swp=x1;x1=x2;x2=swp;swp=y1;y1=y2;y2=swp;}
	            
	            //World X position
	            wx[0]=x1*CS-y1*SN;
	            wx[1]=x2*CS-y2*SN;
	            wx[2]=wx[0];
	            wx[3]=wx[1];
	            
	            //World Y position
	            wy[0]=y1*CS+x1*SN;
                wy[1]=y2*CS+x2*SN;
	            wy[2]=wy[0];
	            wy[3]=wy[1];
		    	S[s].d+=dist(0,0, (wx[0]+wx[1])/2, (wy[0]+wy[1])/2 ); //Average and store wall distance
	            
	            //World Z height
	            wz[0]=S[s].z1-P.z+((P.l*wy[0])/32.0);
	            wz[1]=S[s].z1-P.z+((P.l*wy[1])/32.0);
	            wz[2]=wz[0]+S[s].z2;
	            wz[3]=wz[1]+S[s].z2; //top line has new z value
	            
	            if(wy[0]<1 && wy[1]<1){continue;}
	            
	            if(wy[0]<1)
                {
	            	clipBehindPlayer(&wx[0],&wy[0],&wz[0], wx[1],wy[1],wz[1]); //bottom line
	            	clipBehindPlayer(&wx[2],&wy[2],&wz[2], wx[3],wy[3],wz[3]); //top line
	            }
	            
	            if(wy[1]<1)
	            {
	            	clipBehindPlayer(&wx[1],&wy[1],&wz[1], wx[0],wy[0],wz[0]); //bottom line
	            	clipBehindPlayer(&wx[3],&wy[3],&wz[3], wx[2],wy[2],wz[2]); //top line
	            }
	            
	            //Screen X, Screen Y position
	            wx[0]=wx[0]*200/wy[0]+SW2; wy[0]=wz[0]*200/wy[0]+SH2;
	            wx[1]=wx[1]*200/wy[1]+SW2; wy[1]=wz[1]*200/wy[1]+SH2;
	            wx[2]=wx[2]*200/wy[2]+SW2; wy[2]=wz[2]*200/wy[2]+SH2;
	            wx[3]=wx[3]*200/wy[3]+SW2; wy[3]=wz[3]*200/wy[3]+SH2;
	            
	            //Draw Points
	            //if(wx[0]>0 && wx[0]<SW && wy[0]>0 && wy[0]<SH){pixel(wx[0],wy[0],0);}
	            //if(wx[1]>0 && wx[1]<SW && wy[1]>0 && wy[1]<SH){pixel(wx[1],wy[1],0);}
                drawWall(wx[0],wx[1],wy[0],wy[1], wy[2],wy[3],W[w].c, s);
		    }
			
			S[s].d/=(S[s].we-S[s].ws); //find average sector distance
			S[s].surface*=-1;          //flip to negative to draw surface
			
		}
		
	}
}

void display()
{
	float slider = osGet3DSliderState();
	float iod = slider/3;
	clearBackground();
	if (iod > 0.0f)
	{
        clearBackgroundr();
	}
	draw3D();
	
	gfxFlushBuffers();
	gfxSwapBuffers();	
}

int loadSectors[]=
{//wall start,wall end,z1 height,z2 height,bottom clor,top color
    0,8, 40,50, 9,9,
    8,16, 0,40, 6,6,
    16,24, 80,110, 0,0,
    24,28, 0,30, 6,6,
    28,32, 0,20, 6,6,
    32,36, 0,10, 6,6,
    36,40, 0,30, 5,5,
    40,44, 0,30, 5,5,
    44,48, 30,110, 0,0,
    48,52, 30,110, 0,0,
};

int loadWalls[]=
{//x1,y1,x2,y2,color
    160,228, 168,228, 4,
    168,228, 176,236, 5,
    176,236, 176,244, 4,
    176,244, 168,252, 5,
    168,252, 160,252, 4,
    160,252, 152,244, 5,
    152,244, 152,236, 4,
    152,236, 160,228, 5,
    104,224, 152,184, 1,
    152,184, 176,184, 3,
    176,184, 224,224, 1,
    224,224, 224,256, 0,
    224,256, 192,288, 1,
    192,288, 136,288, 0,
    136,288, 104,256, 1,
    104,256, 104,224, 0,
    104,224, 152,184, 1,
    152,184, 176,184, 0,
    176,184, 224,224, 1,
    224,224, 224,256, 0,
    224,256, 192,288, 1,
    192,288, 136,288, 0,
    136,288, 104,256, 1,
    104,256, 104,224, 0,
    152,168, 176,168, 2,
    176,168, 176,184, 3,
    176,184, 152,184, 2,
    152,184, 152,168, 3,
    152,152, 176,152, 2,
    176,152, 176,168, 3,
    176,168, 152,168, 2,
    152,168, 152,152, 3,
    152,136, 176,136, 2,
    176,136, 176,152, 3,
    176,152, 152,152, 2,
    152,152, 152,136, 3,
    208,160, 208,136, 5,
    208,136, 232,136, 4,
    232,136, 232,160, 5,
    232,160, 208,160, 4,
    96,136, 120,136, 4,
    120,136, 120,160, 5,
    120,160, 96,160, 4,
    96,160, 96,136, 5,
    216,144, 224,144, 4,
    224,144, 224,152, 5,
    224,152, 216,152, 4,
    216,152, 216,144, 5,
    104,144, 112,144, 4,
    112,144, 112,152, 5,
    112,152, 104,152, 4,
    104,152, 104,144, 5,
};

void init()
{int x;
    for (x=0;x<360;x++)
	{
		M.cos[x]=cos(x/180.0*M_PI);
		M.sin[x]=sin(x/180.0*M_PI);
	}
	
	P.x=70; P.y=-110; P.z=20; P.a=0; P.l=0;
	
	int s,w,v1=0,v2=0;
	for(s=0;s<numSect;s++)
	{
		S[s].ws=loadSectors[v1+0];                      //wall start number
		S[s].we=loadSectors[v1+1];                      //wall end number
		S[s].z1=loadSectors[v1+2];                      //sector bottom height
		S[s].z2=loadSectors[v1+3]-loadSectors[v1+2];    //sector top height
		S[s].c1=loadSectors[v1+4];
		S[s].c2=loadSectors[v1+5];
        v1+=6;
	
	    for(w=S[s].ws;w<S[s].we;w++)
	    {
		W[w].x1=loadWalls[v2+0]; //bottom x1
		W[w].y1=loadWalls[v2+1]; //bottom y1
		W[w].x2=loadWalls[v2+2]; //top x2
		W[w].y2=loadWalls[v2+3]; //top y2
		W[w].c =loadWalls[v2+4]; //wall color
		v2+=5;
		}
	}
}

int main()
{
	gfxInitDefault();
	gfxSet3D(true);
	
	consoleInit(GFX_BOTTOM, NULL);
	
	printf("3DS version by Joel,");
	printf("\nBig thanks to 3DSage.");
	printf("\n ");
	printf("\nCPAD: Look");
	printf("\nDPAD UP/Down: Fly UP/Down");
	printf("\nABXY: Movement");
	printf("\nSTART: Exit");
	
    init();
	
	bool running = true;

	while (running) {
		
		gspWaitForVBlank();
		
		movePlayer();
		display();
		
		//exit if start button is pressed
		hidScanInput();
		u32 kDown = hidKeysDown();
		if (kDown & KEY_START)
		    break;
		
	}

	gfxExit();
	return 0;
}
