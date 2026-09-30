// Decompiled by GPT-6, finished by deepseek-v4.1-flash, finished by
// GPT-6.1-sol, finished by space-bunny-free. Names are provisional.
// Partial 51.6%. This is a textured/gouraud triangle rasteriser into 10-int
// span rows, then one FUN_004c8020 call per scanline. The sibling 0x4c8760 is
// the same algorithm without the light channel and is matched to 67.9%; its
// first edge loop uses the same `previous`/`next` idiom adopted here.
//
// Still differs, and it is one systematic cause plus the prologue:
//  - frame is 0x7d5c vs original 0x7d60. The original keeps one more live int
//    in the low temp region: its `defaults`/`spans` start at [esp+0x50] where
//    ours start at [esp+0x4c], so every default, spans and argument offset is
//    4 lower. The original has 16 scalar slots (0x10..0x4c) and this file has
//    15 (0x10..0x48); the difference is NOT only the extra `index-1` slot.
//    Traced slot by slot, the original spends 11 slots on the edge loops
//    (0x10 nxt/dl, 0x14 next/dz, 0x18 lowX/n, 0x1c y1, 0x20 y0, 0x24 x,
//    0x2c out, 0x38 dv, 0x3c du, 0x40 dx, 0x44 bottom, plus 0x4c previous)
//    and 5 on the min/max scan (0x18 lowX, 0x28 highY, 0x30 lowIndex,
//    0x34 lowY, 0x48 highIndex) with highX in NO slot at all: `edi` holds it,
//    from `mov edi, 0xfff0bdc1` with no store, through `cmp eax,edi` in the
//    scan, to `test edi,edi` after it. This file instead spends 6 on the scan
//    (it gives highX a slot at 0x10) and folds `lowX` onto the edge loop's `x`
//    at 0x18. So the missing slot is a NET of two: `previous` must gain one
//    AND highX must lose one.
//  - the min/max loop: original keeps lowY in esi, highX in edi, lowX in ebp
//    and reloads `vertices` from its argument slot; ours keeps `vertices` in
//    esi and spills highX/lowX/both. That is an allocator decision, not a
//    source-shape one that reordering fixed.
//  - the null checks use edx as the zero register; the original zeroes ebp
//    (`xor ebp,ebp` between push ebp and push esi). This is the one upstream
//    cause worth chasing: because the original's zero is ebp, its `vertices`
//    argument stays in edx all the way into the min/max scan and becomes the
//    scan's induction variable directly (`add edx, 0x10`, no reload), so
//    `vertices` costs no second register. Here edx holds the zero, `vertices`
//    must be copied into ebp and then into edx (`mov edx, ebp`), and that one
//    extra live register is what demotes highX out of edi into a stack slot.
//    Nothing reachable from the null checks moved MSVC's choice of zero
//    register; see the ruled-out list.
//
// Ruled out (measured with free --sym scratch runs): moving `bottom` before
// the guard, early-return null checks, function-scope min/max declarations,
// a currentVertex pointer, and prepending the real 0x4c8760 (compiler state).
// tools/headers.py finds no header set that changes the bytes, and 4 to 64
// unused `extern int` declarations leave the score at exactly 51.5%.
// Also tried here: growing defaults to 9 or 10 ints and biasing its pointer
// by one did not align both arrays; those scored 39.9%, 37.7%, and 47.2%.
// Reordering declarations and taking the address of `previous` were neutral.
// Hoisting the block-scope `x`/`y1` (the sibling 0x4c8760's
// winning change) and hoisting the span `x`/`y1` plus porting its whole loop
// structure; those scored 51.3 and 36.1 and did not grow the frame either.
// Also measured here, all free --sym runs, none better than 51.6%:
//  - the sibling 0x4c8760 first-loop shape verbatim, `int previous=index-1`
//    plus `index=previous; if(index<0) index=3;` at the latch (the original's
//    `mov edx,[esp+0x4c]` / `mov edi,edx` / `test` / `mov edi,3` / `cmp`):
//    48.2%, frame still 0x7d5c. It DOES give `previous` its own slot, but
//    only by stealing highX's, and it also makes lowX fold onto `x`, so the
//    count stays at 15. The same as a function-scope `previous` (48.2%) and
//    as `previous<0?3:previous` in both places (48.0%).
//  - rewriting the whole edge body around `currentVertex`/`nextVertex` local
//    pointers (which is how the original's `lea ebp,[ecx+esi]` / `add eax,ecx`
//    pair is shaped): 42.4%, 1284 bytes.
//  - min/max scan as a pointer walk (`int* v=vertices+i*4;`, or a function
//    scope `vp` with `vp+=4`), `if (target) if (texture) if (vertices)`,
//    `!=0` spellings, all six orders of the lowY/highY/highX/lowX
//    initialisers, four separate statements instead of one declaration list,
//    a throwaway live `walk` and `spare` int in the scan, and taking the
//    address of `index`: every one is exactly 51.6% and leaves both the
//    allocation and the frame untouched, so none of them is a lever.
//  - hoisting `n` to function scope shared by both edge loops: 37.8% on top
//    of the previous/next split, 51.6% on its own. Hoisting `x`/`y0`/`y1`/`n`
//    together with the previous/next split: 47.7%.
//  - one C++ header in front, by hand: <string>, <vector>, <map>, <list> and
//    <iostream> are all exactly 51.6%.
//
// Fixes that did land: second edge loop writes ints 1,4,5,7,9 of the span row
// (the row is [xL,xR,uL,vL,uR,vR,zL,zR,lL,lR]), the rasterise guard is
// span[1]-span[0]>0 (not !=0 && >=0), and the loop temps are function-scope.
struct Surface_4c8bb0 { unsigned short width, height; };
void __stdcall FUN_004c8020(int, int*, Surface_4c8bb0*, Surface_4c8bb0*);

// FUNCTION: 0x4c8bb0
void __stdcall FUN_004c8bb0(Surface_4c8bb0* target, Surface_4c8bb0* texture, int* vertices, int* coords)
{
    int defaults[8];
    int spans[800][10];
    int index, previous;
    int* out;
    if (target && texture && vertices) {
        if (!coords) {
            coords=defaults;
            defaults[0]=0; defaults[1]=0;
            defaults[2]=texture->width-1; defaults[3]=0;
            defaults[4]=texture->width-1; defaults[5]=texture->height-1;
            defaults[6]=0; defaults[7]=texture->height-1;
        }
        int lowY=999999, highY=-999999, highX=-999999, lowX=999999;
        int lowIndex, highIndex;
        for(int i=0;i<4;i++) {
            int y=vertices[i*4+1];
            if(y<lowY) { lowY=y; lowIndex=i; }
            if(y>highY) { highY=y; highIndex=i; }
            int x=vertices[i*4];
            if(x>highX) highX=x;
            if(x<lowX) lowX=x;
        }
        if(highX>=0 && lowX<=target->width-1 && highY>=0 && lowY<=target->height-1) {
            int bottom=target->height-1;
            if(lowY<0) lowY=0;
            if(highY>bottom) highY=bottom;
            if(highY!=lowY) {
                {
                    out=&spans[0][0];
                    index=lowIndex;
                    do {
                        previous=index-1;
                        int next=previous;
                        if(next<0) next=3;
                        int* nextVertex=vertices+next*4;
                        int y0=vertices[index*4+1];
                        int y1=nextVertex[1];
                        if (y1>0 && y0<y1) {
                            int dy=y1-y0;
                            int dx=((nextVertex[0]-vertices[index*4])*0x10000)/dy;
                            int x=vertices[index*4]*0x10000+0xffff;
                            int u=coords[index*2]*0x10000;
                            int v=coords[index*2+1]*0x10000;
                            int z=vertices[index*4+2]*0x10000;
                            int light=vertices[index*4+3]*0x10000;
                            int du=(coords[next*2]*0x10000-u)/dy;
                            int dv=(coords[next*2+1]*0x10000-v)/dy;
                            int dz=(nextVertex[2]*0x10000-z)/dy;
                            int dl=(nextVertex[3]*0x10000-light)/dy;
                            if(y0<0) {
                                x-=dx*y0; u-=du*y0; v-=dv*y0; z-=dz*y0;
                                light-=dl*y0;
                                y0=0;
                            }
                            if(y1>bottom) y1=bottom;
                            if(y0<y1) {
                                int n=y1-y0;
                                do {
                                    out[0]=x>>16; x+=dx;
                                    out[2]=u;
                                    u+=du;
                                    out[3]=v;
                                    v+=dv;
                                    out[6]=z;
                                    out[8]=light;
                                    out+=10;

                                    z+=dz;
                                    light+=dl;
                                } while(--n);
                            }
                        }
                        index=next;
                    } while(index!=highIndex);
                }
                {
                    int* out=&spans[0][0];
                    int index=lowIndex;
                    do {
                        int next=(index+1)&3;
                        int* nextVertex=vertices+next*4;
                        int y0=vertices[index*4+1];
                        int y1=nextVertex[1];
                        if (y1>0 && y0<y1) {
                            int dy=y1-y0;
                            int dx=((nextVertex[0]-vertices[index*4])*0x10000)/dy;
                            int x=vertices[index*4]*0x10000+0xffff;
                            int u=coords[index*2]*0x10000;
                            int v=coords[index*2+1]*0x10000;
                            int z=vertices[index*4+2]*0x10000;
                            int light=vertices[index*4+3]*0x10000;
                            int du=(coords[next*2]*0x10000-u)/dy;
                            int dv=(coords[next*2+1]*0x10000-v)/dy;
                            int dz=(nextVertex[2]*0x10000-z)/dy;
                            int dl=(nextVertex[3]*0x10000-light)/dy;
                            if(y0<0) {
                                x-=dx*y0; u-=du*y0; v-=dv*y0; z-=dz*y0;
                                light-=dl*y0;
                                y0=0;
                            }
                            if(y1>bottom) y1=bottom;
                            if(y0<y1) {
                                int n=y1-y0;
                                do {
                                    out[1]=x>>16; x+=dx;
                                    out[4]=u;
                                    u+=du;
                                    out[5]=v;
                                    v+=dv;
                                    out[7]=z;
                                    out[9]=light;
                                    out+=10;

                                    z+=dz;
                                    light+=dl;
                                } while(--n);
                            }
                        }
                        index=next;
                    } while(index!=highIndex);
                }
                int* span=&spans[0][0];
                for(int row=lowY;row<highY;row++) {
                    if(span[1]-span[0]>0)
                        FUN_004c8020(row,span,target,texture);
                    span+=10;
                }
            }
        }
    }
}
