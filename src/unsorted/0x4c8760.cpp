// Decompiled by GPT-6, finished by deepseek-v4.1-flash, finished by
// space-bunny-free, finished by GPT-6.1-sol, edited by deepseek-v4.1. Names are provisional.
// PARTIAL, 72.8% (1094 original bytes, 1094 ours).
// The min/max scan and both span-edge loops now emit the same instruction
// set; every remaining diff is stack frame layout and instruction scheduling.
// Fixed here: the second span loop writes its u/v through offsets 4 and 5
// (out = &spans[0][0], out[1]=x, out[4]=u, out[5]=v, out[7]=z), not 2/3 as a
// first reading of the old source suggested; that is what took it from 67.9
// to 72.5 and made the byte count exact.
// What still differs:
// 1. Frame is 0x7d54, original 0x7d58: exactly one 4-byte scalar slot short
//    (13 slots at 0x10..0x40 vs the original's 14 at 0x10..0x44, so every
//    [esp+N] differs). The original's ascending slot order is next, lowX, x,
//    y1, highY, lowY, lowIndex, du, out, dx, nextVertex, bottom, highIndex,
//    previous; MSVC did NOT share lowX with next/dv there (next=0x10,
//    lowX=0x14 shared with loop1 n and loop2 next), while ours shares lowX
//    with next/dv and puts y1 at 0x14. Declaring all fourteen scalars at
//    function scope in exactly that order gave 72.8% but the frame stayed
//    0x7d54, so the allocator is coloring by live range, not declaration
//    order; the levers left are the loop-body locals' scopes (n, dv, dz) that
//    decide the reuse pairs.
// 2. First span loop body order: the original emits `x+=dx` right after
//    `out[0]=x>>16`, before `out[2]=u`; ours emits it after `out+=10`.
// 3. Second span loop exit: original does `mov edi,eax; mov eax,[highIndex];
//    cmp edi,eax`; ours uses ecx for highIndex and `cmp eax,ecx`.
// Tried a rowEnd temporary, reversing lowIndex/highIndex declaration order,
// and rewriting the final for loop as while; none changed the 72.5% score.
// The 128-set header sweep also found no improvement. A remaining approach is
// to find the declaration/scope construct that makes the allocator give lowX
// its own slot instead of folding it onto next, and makes the second loop's n
// reuse lowIndex.
struct Surface_4c8760 { unsigned short width, height; };
void __stdcall FUN_004c7a20(int, int*, Surface_4c8760*, Surface_4c8760*);

// FUNCTION: 0x4c8760
void __stdcall FUN_004c8760(Surface_4c8760* target, Surface_4c8760* texture, int* vertices, int* coords)
{
    int defaults[8];
    int spans[800][10];
    int next;
    int lowX;
    int x;
    int y1;
    int highY;
    int lowY;
    int lowIndex;
    int du;
    int* out;
    int dx;
    int* nextVertex;
    int bottom;
    int highIndex;
    int previous;
    int highX;
    if (target && texture && vertices) {
        if (!coords) {
            coords=defaults;
            defaults[0]=0; defaults[1]=0;
            defaults[2]=texture->width-1; defaults[3]=0;
            defaults[4]=texture->width-1; defaults[5]=texture->height-1;
            defaults[6]=0; defaults[7]=texture->height-1;
        }
        lowY=999999; highY=-999999; highX=-999999; lowX=999999;
        for(int i=0;i<4;i++) {
            int vy=vertices[i*3+1];
            if(vy<lowY) { lowY=vy; lowIndex=i; }
            if(vy>highY) { highY=vy; highIndex=i; }
            int vx=vertices[i*3];
            if(vx>highX) highX=vx;
            if(vx<lowX) lowX=vx;
        }
        if(highX>=0 && lowX<=target->width-1 && highY>=0 && lowY<=target->height-1) {
            bottom=target->height-1;
            if(lowY<0) lowY=0;
            if(highY>bottom) highY=bottom;
            if(highY!=lowY) {
                {
                    out=&spans[0][0];
                    int index=lowIndex;
                    do {
                        previous=index-1;
                        next=previous;
                        if (next<0) next=3;
                        int* currentVertex=vertices+index*3;
                        int y0=currentVertex[1];
                        y1=vertices[next*3+1];
                        if (y0<y1) {
                            int dy=y1-y0;
                            dx=((vertices[next*3]-currentVertex[0])*0x10000)/dy;
                            x=currentVertex[0]*0x10000+0xffff;
                            int z=currentVertex[2]*0x10000;
                            int u=coords[index*2]*0x10000;
                            int v=coords[index*2+1]*0x10000;

                            du=(coords[next*2]*0x10000-u)/dy;
                            int dv=(coords[next*2+1]*0x10000-v)/dy;
                            int dz=(vertices[next*3+2]*0x10000-z)/dy;

                            if(y0<0) {
                                x-=dx*y0; u-=du*y0; v-=dv*y0; z-=dz*y0;

                                y0=0;
                            }
                            if(y1>bottom) y1=bottom;
                            if(y0<y1) {
                                int n=y1-y0;
                                do {
                                    out[0]=x>>16; x+=dx;
                                    out[2]=u;

                                    out[3]=v;

                                    out[6]=z;

                                    out+=10;
                                    u+=du; v+=dv;
                                    z+=dz;

                                } while(--n);
                            }
                        }
                        index=previous;
                        if(index<0) index=3;
                    } while(index!=highIndex);
                }
                {
                    out=&spans[0][0];
                    int index=lowIndex;
                    do {
                        next=(index+1)&3;
                        int* currentVertex=vertices+index*3;
                        int y0=currentVertex[1];
                        y1=vertices[next*3+1];
                        nextVertex=vertices+next*3;
                        if (y0<y1) {
                            int dy=y1-y0;
                            dx=((nextVertex[0]-currentVertex[0])*0x10000)/dy;
                            x=currentVertex[0]*0x10000+0xffff;
                            int z=currentVertex[2]*0x10000;
                            int u=coords[index*2]*0x10000;
                            int v=coords[index*2+1]*0x10000;

                            du=(coords[next*2]*0x10000-u)/dy;
                            int dv=(coords[next*2+1]*0x10000-v)/dy;
                            int dz=(nextVertex[2]*0x10000-z)/dy;

                            if(y0<0) {
                                x-=dx*y0; u-=du*y0; v-=dv*y0; z-=dz*y0;

                                y0=0;
                            }
                            if(y1>bottom) y1=bottom;
                            if(y0<y1) {
                                int n=y1-y0;
                                do {
                                    out[1]=x>>16; x+=dx;
                                    out[4]=u;

                                    out[5]=v;

                                    out[7]=z;

                                    out+=10;
                                    u+=du; v+=dv;
                                    z+=dz;

                                } while(--n);
                            }
                        }
                        index=next;
                    } while(index!=highIndex);
                }
                int* span=&spans[0][0];
                for(int row=lowY;row<highY;row++) {
                    if(span[1]-span[0]>0)
                        FUN_004c7a20(row,span,target,texture);
                    span+=10;
                }
            }
        }
    }
}
