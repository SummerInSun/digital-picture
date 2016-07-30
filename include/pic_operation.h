#ifndef __PIC_OPERATION_H__
#define __PIC_OPERATION_H__

int PicZoomOpr(struct PiexlDatasDesc *ptOrigin, struct PiexlDatasDesc *ptZoom);
int PicMergeOpr(int iXPos, int iYPos, struct PiexlDatasDesc *ptSmallPic, struct PiexlDatasDesc *ptBigPic);

#endif

