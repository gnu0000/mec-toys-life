/*
 *
 * life.c
 * Saturday, 5/16/1998.
 * Craig Fitzgerald
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <GnuType.h>
#include <GnuMem.h>
#include <GnuScr.h>
#include <GnuWin.h>
#include <GnuKbd.h>
#include <GnuMisc.h>
#include <GnuFile.h>

#define XSIZ 80
#define YSIZ 50

#define THINGIE (BYTE)'è'
#define BLANK   (BYTE)'ú'

#define ATT_THINGIE 0x02
#define ATT_BLANK   0x01
#define ATT_CURSOR  0x20
#define ATT_SELECT  0x10
#define ATT_MSG     0x1E
#define ATT_TEXT    0x1F

#define OBJECT_COUNT 10
#define SCREEN_COUNT 10

#define MAXSCR_BUFFERS 0x60
#define SLEEP_INTERVAL 200


#define POS  USHORT
#define PPOS PUSHORT
#define pos(x,y) ((USHORT)((((y) & 0xFF) << 8)+((x) & 0xFF)))
#define xpos(p)  ((INT)((p) & 0xFF))
#define ypos(p)  ((INT)(((USHORT)(p)) >> 8))
#define size(p1,p2) pos((xpos(p2)-xpos(p1)+1),(ypos(p2)-ypos(p1)+1))

#define pcell(x,y)  (pcScr + (((y)+YSIZ) % YSIZ) * XSIZ + (((x)+XSIZ) % XSIZ))
#define pcella(x,y) (pcAlt + (((y)+YSIZ) % YSIZ) * XSIZ + (((x)+XSIZ) % XSIZ))
#define pbcell(pb,x,y) ((pb)->pszBuff + ((((y)+(pb)->iYSize)%(pb)->iYSize)*(pb)->iXSize + (((x)+(pb)->iXSize)%(pb)->iXSize)))

#define MAX_AUTO_STEPS        2500

//BYTE cThingie[2] = {THINGIE, ATT_THINGIE};
//BYTE cBlank[2]   = {BLANK  , ATT_BLANK  };

CHAR_INFO cThingie;
CHAR_INFO cBlank	;

INT iBUFIDX = 0;
INT SCREEN_BUFFERS;
PSZ pcScr, ppcBuffers[MAXSCR_BUFFERS];

INT  iDELAY         = 120;
INT  iAUTOFINDDELAY = 0;

UINT uITER = 0;

CHAR sz[4096];

/***************************************************************************/
/*                                                                         */
/***************************************************************************/

/*
 * call with yTop < 0 to repaint entire screen
 *
 */
void Display (INT yTop, INT xLeft, INT yBottom, INT xRight)
   {
   PSZ   pPos;
   INT   y, x;

   if (yTop < 0)
      {
      pPos = pcScr;
      for (y=0; y<YSIZ; y++)
         for (x=0; x<XSIZ; x++)
            {
            ScrWriteNCell ((*pPos ? cThingie : cBlank), 1, y, x);
            pPos++;
            }
      }
   else
      {
      for (y=yTop; y<=yBottom; y++)
         for (x=xLeft; x<=xRight; x++)
            ScrWriteNCell ((*pcell (x,y) ? cThingie : cBlank), 1, y, x);
      }
   }

void DisplayIter (void)
   {
   sprintf (sz, "%5.5u", uITER);
   ScrWriteStrAtt (sz, 5, YSIZ-1, XSIZ-5, ATT_THINGIE);
   }

void Set (INT iX, INT iY, INT iCmd)
   {
   PSZ pc;

   pc  = pcell (iX, iY);
   *pc = (iCmd < 2 ? !!iCmd : !*pc);
   Display (iY, iX, iY, iX);
   }


UINT UserKey (PSZ pszMsg)
   {
   UINT c;

   ScrWriteStrAtt (pszMsg, strlen (pszMsg), YSIZ-1, 10, ATT_MSG);
   c = KeyGet (TRUE);
   Display (YSIZ-1, 10, YSIZ-1, XSIZ-1);
   return c;
   }

void Order (PPOS ppos1, PPOS ppos2)
   {
   POS posMin, posMax;

   posMin = pos (min (xpos (*ppos1), xpos (*ppos2)), min (ypos (*ppos1), ypos (*ppos2)));
   posMax = pos (max (xpos (*ppos1), xpos (*ppos2)), max (ypos (*ppos1), ypos (*ppos2)));
   *ppos1 = posMin;
   *ppos2 = posMax;
   }

void RandomSize (PPOS ppos1, PPOS ppos2, UINT uSize)
   {
   POS pSiz;

   if (!uSize)
      pSiz = pos (3 + Rnd (7),  3 + Rnd (7) );
   else
      pSiz = pos (1 + Rnd (33), 1 + Rnd (23));

   *ppos1 = pos ((XSIZ - xpos(pSiz))/2,   (YSIZ - ypos(pSiz))/2  );
   *ppos2 = pos ((XSIZ + xpos(pSiz))/2-1, (YSIZ + ypos(pSiz))/2-1);
   }



void MySleep (UINT uWait)
   {
   UINT i, uInterval;

   for (i=0; !k_kbhit () && i < uWait; i+=uInterval)
      {
      uInterval = min (SLEEP_INTERVAL, uWait - i);
      FineSleep (uInterval);
      }
   }


/***************************************************************************/
/*                                                                         */
/***************************************************************************/

INT iTextPos;

void pr (PSZ psz)
   {
   ScrWriteStrAtt (psz, strlen (psz), iTextPos++, 10, ATT_TEXT);
   }

void Commands (void)
   {
   iTextPos = 5;

   sprintf (sz, "                                       Buffers:%3.3d", SCREEN_BUFFERS);
   pr (sz);

   pr (" [In Edit Mode]                                   ");
   pr ("    <Esc> ............ Exit Mark Mode or Quit     ");
   pr ("    <Alt-X>........... Quit                       ");
   pr ("    <G>............... Generate Mode              ");
   pr ("    <+>............... Generate Step              ");
   pr ("    <->............... Backup (up to N steps)     ");
   pr ("    <Arrow.Keys>...... Move Cursor                ");
   pr ("    <Ctl-<Arrow.Keys>. Move Screen                ");
   pr ("    <Ins>............. Fill Mode Toggle           ");
   pr ("    <Del>............. Erase Mode Toggle          ");
   pr ("    <Ret>............. Fill at cursor             ");
   pr ("    <Spc>............. Fill/Erase at cursor       ");
   pr ("    <Fn>.............. Copy Screen                ");
   pr ("    <Shift-Fn>........ Paste Screen               ");
   pr ("    <F11>, <Alt-M>.... Start/End Mark Mode        ");
   pr ("    <F12>............. End Mark Mode              ");
   pr ("    <Ctl-Fn>.......... Paste Object               ");
   pr ("    <Alt-R>........... Random Fill Marked Area    ");
   pr ("    <Alt-C>........... Clear Marked Area/Screen   ");
   pr ("    <Alt-L>........... Restore Last Screen        ");
   pr ("    <Alt-F> .......... Auto Find Repeaters        ");
   pr ("    <Alt-I> .......... Mirror Object or Screen    ");
   pr ("    <L> .............. Load Object Dat File       ");
   pr ("                                                  ");
   pr (" [In Generate Mode]                               ");
   pr ("    <Esc> ............ Edit Mode                  ");
   pr ("    <+> .............. Faster                     ");
   pr ("    <-> .............. Slower                     ");
   pr ("                                                  ");
   pr (" [In Auto Find Mode]                              ");
   pr ("    <Esc> ............ Edit Mode                  ");
   pr ("    <+> .............. Faster                     ");
   pr ("    <-> .............. Slower                     ");
   pr ("    <K> .............. Keep                       ");
   pr ("    <S> .............. Skip                       ");
   pr ("    <Q>, <F>.......... Toggle Quiet/Show Mode     ");
   pr ("                                                  ");
   pr ("    [Press any key to start AutoFind mode]        ");
   pr ("                                                  ");

   KeyGet (TRUE);
   Display (-1, 0, 0, 0);
   }

/***************************************************************************/
/*                                                                         */
/***************************************************************************/

//void Evolve (void)
//   {
//   PSZ pcAlt;
//   INT y, x, yd, xd, iCt;
//
//   pcAlt = ppcBuffers [(iBUFIDX + 1) % SCREEN_BUFFERS];
//
//   for (y=0; y<YSIZ; y++)
//      {
//      for (x=0; x<XSIZ; x++)
//         {
//         iCt = 0;
//         for (yd=-1; yd<2; yd++)
//            for (xd=-1; xd<2; xd++)
//               iCt += *pcell (x+xd, y+yd);
//         pcAlt [y*XSIZ + x] = !!((iCt == 3) || (iCt == 4 && *pcell (x, y)));
//         }
//      }
//   }

/*
 * Speed optimized version of above fn
 *
 */
void Evolve (void)
   {
   PSZ pcAlt;
   INT y, x, yd, iCt, iCtL, iCtM, iCtR;

   pcAlt = ppcBuffers [(iBUFIDX + 1) % SCREEN_BUFFERS];

   for (y=0; y<YSIZ; y++)
      {
      iCtM = iCtR = 0; 

      for (yd=-1; yd<2; yd++)
         {
         iCtM += *pcell (XSIZ-1, y+yd);
         iCtR += *pcell (0, y+yd);
         }

      for (x=0; x<XSIZ; x++)
         {
         iCtL = iCtM; 
         iCtM = iCtR;
         iCtR = 0;

         for (yd=-1; yd<2; yd++)
            iCtR += *pcell (x+1, y+yd);

         iCt = iCtL + iCtM + iCtR;
         *pcAlt++ = !!((iCt == 3) || (iCt == 4 && *pcell (x, y)));
         }
      }
   uITER++;
   }



void GenStep (BOOL bQuietly)
   {
   Evolve ();

   iBUFIDX = (iBUFIDX+1) % SCREEN_BUFFERS;
   pcScr = ppcBuffers [iBUFIDX];

   if (!bQuietly)
      Display (-1, 0, 0, 0);
   }


void GenMode (void)
   {
   UINT c;

   while (TRUE)
      {
      GenStep (FALSE);

      while (k_kbhit ())
         {
         switch (c = KeyGet (TRUE))
            {
            case '+':
               iDELAY = max (0, iDELAY - max (10, iDELAY >> 2));
               break;
            case '-':
               iDELAY = min (10000, iDELAY + max (10, iDELAY >> 2));
               break;
            default:
               return;
            }
         }
      MySleep (iDELAY);
      }
   }

/***************************************************************************/
/*                                                                         */
/***************************************************************************/

void WriteAtt (INT iX, INT iY, INT iAtt)
   {
   BYTE pCel[2];
   BOOL bThingie;

   bThingie = *pcell (iX, iY);

   pCel[0] = (bThingie ? THINGIE : BLANK);
   pCel[1] = (BYTE)(iAtt ? iAtt : (bThingie ? ATT_THINGIE : ATT_BLANK));
   ScrWriteNCell (pCel, 1, iY, iX);
   }


void ShowCursor (POS pos, BOOL bShow)
   {
   INT iX, iY;

   iX = xpos (pos);
   iY = ypos (pos);

   WriteAtt (iX, iY, (bShow ? ATT_CURSOR : 0));
   }

void ShowMarkOutlines (POS pos1, POS pos2, BOOL bShow)
   {
   INT  x, y;
   BYTE bAtt;

   Order (&pos1, &pos2);
   bAtt = (bShow ? ATT_SELECT : 0);

   for (x = xpos(pos1); x <= xpos(pos2); x++)
      WriteAtt (x, ypos (pos1), bAtt), WriteAtt (x, ypos(pos2), bAtt);

   for (y = ypos(pos1); y <= ypos(pos2); y++)
      WriteAtt (xpos(pos1), y, bAtt), WriteAtt (xpos(pos2), y, bAtt);
   }


void RollScreen (INT iXDelta, INT iYDelta)
   {
   PSZ pcAlt;
   INT y, x;

   pcAlt = ppcBuffers [(iBUFIDX + 1) % SCREEN_BUFFERS];

   for (y=0; y<YSIZ; y++)
      for (x=0; x<XSIZ; x++)
         *pcella (x+iXDelta, y+iYDelta) = *pcell (x, y);

   iBUFIDX = (iBUFIDX+1) % SCREEN_BUFFERS;
   pcScr = ppcBuffers [iBUFIDX];
   Display (-1, 0, 0, 0);
   }


void CopyBlock (PSZ pszName, POS pos1, POS pos2)
   {
   FILE *fp;
   POS  pSiz;
   INT  y, x;

   if (!(fp = fopen (pszName, "wt")))
      Error ("Could not open %s", pszName);

   pSiz = size (pos1, pos2);
   fprintf (fp, "%2.2d %2.2d\n", xpos (pSiz), ypos (pSiz));

   for (y=ypos(pos1); y<=ypos(pos2); y++)
      {
      for (x=xpos(pos1); x<=xpos(pos2); x++)
         fputc ((*pcell (x,y) ? 'x' : ' '), fp);
      fputc ('\n', fp);
      }
   fputc ('\n', fp);
   fclose (fp);
   }


int PasteBlock (PSZ pszName, POS pos)
   {
   FILE *fp;
   POS  pSiz;
   INT  y, x;

   if (!(fp = fopen (pszName, "rt")))
      return GNUBeep (0);

   if (FilReadLine (fp, sz, ";", sizeof (sz)) == (UINT)-1)
      return GNUBeep (0);

   pSiz = pos (atoi(sz), atoi(sz+3));

   for (y=0; y<ypos(pSiz); y++)
      {
      memset (sz, 0, sizeof (sz));
      if (FilReadLine (fp, sz, ";", sizeof (sz)) == (UINT)-1)
         return GNUBeep (0);
      for (x=0; x<xpos(pSiz); x++)
         *pcell (x+xpos(pos), y+ypos(pos)) = (sz[x] && sz[x] != ' ');
      }
   fclose (fp);
   Display (ypos(pos), xpos(pos), ypos(pos)+ypos(pSiz)-1, xpos(pos)+xpos(pSiz)-1);
   return 0;
   }   


void ClearBlock (POS pos1, POS pos2)
   {
   INT x, y;

   Order (&pos1, &pos2);
   for (y = ypos(pos1); y <= ypos(pos2); y++)
      for (x = xpos(pos1); x <= xpos(pos2); x++)
         *pcell (x, y) = 0;
   Display (ypos(pos1), xpos(pos1), ypos(pos2), xpos(pos2));
   }


/*
 * uCmd: 1 - Left/Right Reflection
 *       2 - Left/Right Backwards Reflection
 *       3 - Top/Bottom Reflection
 *       4 - Top/Bottom Backwards Reflection
 */
VOID MirrorObject (POS pos1, POS pos2, UINT uCmd, BOOL bDisplay)
   {
   INT y, x;
   POS pSiz;

   Order (&pos1, &pos2);
   pSiz = size (pos1, pos2);

   if (uCmd == 1)       // Left/Right Reflection
      {
      for (y=0; y<ypos(pSiz); y++)
         for (x=0; x<xpos(pSiz)/2; x++)
            *pcell (xpos(pos2)-x, ypos(pos1)+y) = *pcell (xpos(pos1)+x, ypos(pos1)+y);
      }
   else if (uCmd == 2)  // Left/Right Backwards Reflection
      {
      for (y=0; y<ypos(pSiz); y++)
         for (x=0; x<xpos(pSiz)/2; x++)
            *pcell (xpos(pos2)-x, ypos(pos2)-y) = *pcell (xpos(pos1)+x, ypos(pos1)+y);

      if (xpos(pSiz) % 2)
         {
         x = (xpos(pos1)+xpos(pos2))/2;
         for (y=0; y<ypos(pSiz)/2; y++)
            *pcell (x, ypos(pos2)-y) = *pcell (x, ypos(pos1)+y);
         }
      }
   else if (uCmd == 3)  // Top/Bottom Reflection
      {
      for (y=0; y<ypos(pSiz)/2; y++)
         for (x=0; x<xpos(pSiz); x++)
            *pcell (xpos(pos1)+x, ypos(pos2)-y) = 
            *pcell (xpos(pos1)+x, ypos(pos1)+y);
      }
   else if (uCmd == 4)  // Top/Bottom Backwards Reflection
      {
      for (y=0; y<ypos(pSiz)/2; y++)
         for (x=0; x<xpos(pSiz); x++)
            *pcell (xpos(pos2)-x, ypos(pos2)-y) = 
            *pcell (xpos(pos1)+x, ypos(pos1)+y);

      if (ypos(pSiz) % 2)
         {
         y = (ypos(pos1)+ypos(pos2))/2;
         for (x=0; x<xpos(pSiz)/2; x++)
            *pcell (xpos(pos2)-x, y) = *pcell (xpos(pos1)+x, y);
         }
      }
   if (bDisplay)
      Display (ypos(pos1), xpos(pos1), ypos(pos2), xpos(pos2));
   }



void MakeRandomStart (POS pos1, POS pos2)
   {
   POS  pSiz;                                            
   UINT i, uSteps, uStraight, uTurn1, uTurn2; 
   UINT uTurnAny, uChanceSpace, uRoll;
   INT  y, x, iDir, iPlotChance, iTurn, iPrevTurn = 0;

   Order (&pos1, &pos2);
   pSiz = size (pos1, pos2);

   if (Rnd(2))   // speckle generate
      {
      iPlotChance = 20 + Rnd (55);

      for (y=ypos(pos1); y<ypos(pos2); y++)
         for (x=xpos(pos1); x<xpos(pos2); x++)
            *pcell (x, y) = (Rnd(100) < iPlotChance);
      }
   else                      // trace generate
      {
      /*--- clear out first ---*/
      for (y=ypos(pos1); y<ypos(pos2); y++)
         for (x=xpos(pos1); x<xpos(pos2); x++)
            *pcell (x, y) = 0;

      x = xpos (pSiz) /2, y = ypos (pSiz) /2;
      iDir = 0;

      uSteps = (xpos(pSiz) * ypos(pSiz) * (10 + Rnd (55))) / 100;
      uStraight    = 1 + Rnd (100);
      uTurn1       = 1 + Rnd (60);
      uTurn2       = 1 + Rnd (40);
      uTurnAny     = 1 + Rnd (20);
      uChanceSpace = uStraight + uTurn1 + uTurn2 + uTurnAny;
      iPlotChance  = 70 + Rnd (30);

      for (i=0; i<uSteps; i++)
         {
         uRoll = Rnd (uChanceSpace);
         if (uRoll < uTurnAny)
            {
            iDir = Rnd (8);
            }
         else if (uRoll - uTurnAny < uTurn2)
            {
            iTurn = (Rnd (2) ? iPrevTurn : (Rnd (2) ? 2 : -2));
            iPrevTurn = iTurn;
            iDir = (iDir + iTurn + 8) % 8;
            }
         else if (uRoll - uTurnAny - uTurn2 < uTurn1)
            {
            iTurn = (Rnd (2) ? iPrevTurn : (Rnd (2) ? 1 : -1));
            iPrevTurn = iTurn;
            iDir = (iDir + iTurn + 8) % 8;
            }
         switch (iDir)
            {
            case 0: x = (x+1) % xpos(pSiz);                                               break;
            case 1: x = (x+1) % xpos(pSiz);            y = (y+ypos(pSiz)-1) % ypos(pSiz); break;
            case 2:                                    y = (y+ypos(pSiz)-1) % ypos(pSiz); break;
            case 3: x = (x+xpos(pSiz)-1) % xpos(pSiz); y = (y+ypos(pSiz)-1) % ypos(pSiz); break;
            case 4: x = (x+xpos(pSiz)-1) % xpos(pSiz);                                    break;
            case 5: x = (x+xpos(pSiz)-1) % xpos(pSiz); y = (y+1) % ypos(pSiz);            break;
            case 6:                                    y = (y+1) % ypos(pSiz);            break;
            case 7: x = (x+1) % xpos(pSiz);            y = (y+1) % ypos(pSiz);            break;
            }
         *pcell (xpos(pos1)+x, ypos(pos1)+y) = (Rnd(100) <= iPlotChance);
         }
      }
   switch (Rnd (7))
      {
      case 0:     // No symmetry
      case 1:
      case 2:
         break;
      case 3:     // Left/Right symmetry
         MirrorObject (pos1, pos2, 1, FALSE);
         break;
      case 4:     // Top/Bottom symmetry
         MirrorObject (pos1, pos2, 3, FALSE);
         break;
      case 5:     // Backward symmetry
         MirrorObject (pos1, pos2, (Rnd(2)? 2 : 4), FALSE);
         break;
      case 6:     // 4-way symmetry
         MirrorObject (pos1, pos2, 1, FALSE);
         MirrorObject (pos1, pos2, 3, FALSE);
         break;
      }
   }



void RFillObject (POS pos1, POS pos2)
   {
   MakeRandomStart (pos1, pos2);
   Display (ypos(pos1), xpos(pos1), ypos(pos2), xpos(pos2));
   uITER=0;
   }

void ClearObject (POS pos1, POS pos2)
   {
   ClearBlock (pos1, pos2);
   uITER=0;
   }

void CopyObject (POS pos1, POS pos2, INT iIndex)
   {
   Order (&pos1, &pos2);
   sprintf (sz, "OBJECT%d.DAT", iIndex);
   CopyBlock (sz, pos1, pos2);
   }

void AskCopyObject (POS pos1, POS pos2)
   {
   UINT c;
   PSZ  psz;

   Order (&pos1, &pos2);
   c = UserKey (" Copy to which FN key ? (F1 - F10, W, <Esc> ");
   if (c == 'W')
      {
      psz = "Enter Filename:                                               ";
      ScrWriteStrAtt (psz, strlen (psz), YSIZ-1, 0, ATT_MSG);
      if (KeyEditCell (sz, YSIZ-1, 16, 50, 0))
         {
         strcat (sz, (strchr (sz,'.') ? "" : ".dat"));
         CopyBlock (sz, pos1, pos2);
         }
      Display (YSIZ-1, 0, YSIZ-1, XSIZ-1);
      }
   if (c >= K_F1 && c <= K_F10)
      CopyObject (pos1, pos2, c - K_F1);
   }

void PasteObject (POS pos, INT iIndex)
   {
   sprintf (sz, "OBJECT%d.DAT", iIndex);
   PasteBlock (sz, pos);
   uITER=0;
   }

void LoadObject (POS pos)
   {
   CHAR szFile[256];

//   if (!GnuFileWindow (szFile, "FND*.DAT", "Load Object", sz))
   if (!GnuFileWindow (szFile, "*.DAT", "Load Object", sz))
      return;
   PasteBlock (szFile, pos);
   }

void ClearScreen (void)
   {
   ClearBlock (pos (0, 0), pos (XSIZ-1, YSIZ-1));
   uITER=0;
   }

void CopyScreen (INT iIndex)
   {
   sprintf (sz, "SCREEN%d.DAT", iIndex);
   CopyBlock (sz, pos (0, 0), pos (XSIZ-1, YSIZ-1));
   }

void PasteScreen (INT iIndex)
   {
   sprintf (sz, "SCREEN%d.DAT", iIndex);
   PasteBlock (sz, pos (0, 0));
   uITER=0;
   }


/***************************************************************************/
/*                                                                         */
/***************************************************************************/


BOOL Repeating (void)
   {
   PSZ pc1, pc2, pc3;

   pc1 = ppcBuffers [(iBUFIDX + SCREEN_BUFFERS - 0) % SCREEN_BUFFERS];
   pc2 = ppcBuffers [(iBUFIDX + SCREEN_BUFFERS - 2) % SCREEN_BUFFERS];
   pc3 = ppcBuffers [(iBUFIDX + SCREEN_BUFFERS - 3) % SCREEN_BUFFERS];

   if (!memcmp (pc1, pc2, XSIZ * YSIZ))
      return TRUE;
   return !memcmp (pc1, pc3, XSIZ * YSIZ);
   }


void AutoFind (BOOL bQuietly)
   {
   POS  pos1, pos2;
   UINT uRand;
   INT  i;
   BOOL bRepeating;

   while (TRUE)
      {
      ClearScreen ();

      RandomSize (&pos1, &pos2, Rnd (2));
      MakeRandomStart (pos1, pos2);
      CopyBlock ("FIND.DAT", pos1, pos2);
      uITER = 0;

      Display (-1, 0, 0, 0);
      if (!bQuietly)
         MySleep (1000);

      for (bRepeating = FALSE, i=0; !bRepeating && (i<MAX_AUTO_STEPS); i++)
         {
         GenStep (bQuietly);

         if (!bQuietly || !(i % 10))
            DisplayIter ();
         if (!bQuietly)
            MySleep (iAUTOFINDDELAY);

         bRepeating = Repeating ();

         while (k_kbhit ())
            {
            switch (KeyGet (TRUE))
               {
               case '+':
                  iAUTOFINDDELAY = max (0, iAUTOFINDDELAY - max (10, iAUTOFINDDELAY >> 2));
                  break;
               case '-':
                  iAUTOFINDDELAY = min (10000, iAUTOFINDDELAY + max (10, iAUTOFINDDELAY >> 2));
                  break;
               case 'K': // keep
                  i = 32000;
                  break;
               case 'S': // skip
                  i = 32000;
                  bRepeating = TRUE;
                  break;

               case 'Q':
               case 'F':
                  bQuietly = !bQuietly;
                  break;

               default:
                  Display (-1, 0, 0, 0);
                  return;
               }
            }
         }
      if (!bRepeating)
         {
         uRand = rand();
         sprintf (sz, "FND%5.5d.DAT", uRand);
         rename ("FIND.DAT", sz);
         sprintf (sz, "FND%5.5d.NFO", uRand);
         CopyBlock (sz, pos (0, 0), pos (XSIZ-1, YSIZ-1));
         }
      else
         {
         unlink ("FIND.DAT");
         }
      }
   }

void AskQuit (void)
   {
   PMET pmet;

   if (UserKey (" Quit ? [Y, N] ") != 'Y')
      return;

   ScrRestoreScreen ();
   pmet = ScrGetMetrics ();
   ScrClear (0, 0, 0, 0, ' ', pmet->cOriginal);
   exit (0);
   }

void ChangeBuffer (INT iChange)
   {
   iBUFIDX = (iBUFIDX+SCREEN_BUFFERS+iChange) % SCREEN_BUFFERS;
   pcScr = ppcBuffers [iBUFIDX];
   uITER += iChange;
   Display (-1, 0, 0, 0);
   }

/***************************************************************************/
/*                                                                         */
/***************************************************************************/

void EditMode (void)
   {
   BOOL bBreak   = FALSE;
   BOOL bMarking = FALSE;
   UINT c, iX=40, iY=25;
   INT  iSetMode = 0;
   POS  posMarkStart, posHere;

   while (!bBreak)
      {
      posHere = pos (iX, iY);

      if (bMarking) ShowMarkOutlines (posMarkStart, posHere, TRUE);
      ShowCursor (posHere, TRUE);

      c = KeyGet (TRUE);

      if (bMarking) ShowMarkOutlines (posMarkStart, posHere, FALSE);
      ShowCursor (posHere, FALSE);

      switch (c)
         {
         /*--- cursor movement ---*/
         case K_UP   : iY = (iY + YSIZ - 1) % YSIZ; break;
         case K_DOWN : iY = (iY + 1) % YSIZ;        break;
         case K_LEFT : iX = (iX + XSIZ - 1) % XSIZ; break;
         case K_RIGHT: iX = (iX + 1) % XSIZ;        break;
         case K_HOME : iY = (iY + YSIZ - 1) % YSIZ;
                       iX = (iX + XSIZ - 1) % XSIZ; break;
         case K_END  : iY = (iY + 1) % YSIZ;
                       iX = (iX + XSIZ - 1) % XSIZ; break;
         case K_PGUP : iY = (iY + YSIZ - 1) % YSIZ; 
                       iX = (iX + 1) % XSIZ;        break;
         case K_PGDN : iY = (iY + 1) % YSIZ;
                       iX = (iX + 1) % XSIZ;        break;
         case K_PAD5 : iX = XSIZ/2; iY=YSIZ/2;      break;


         /*--- screen movement ---*/
         case K_CUP   : RollScreen ( 0, -1); break;
         case K_CDOWN : RollScreen ( 0,  1); break;
         case K_CLEFT : RollScreen (-1,  0); break;
         case K_CRIGHT: RollScreen ( 1,  0); break;
         case K_CHOME : RollScreen (-1, -1); break;
         case K_CEND  : RollScreen (-1,  1); break;
         case K_CPGUP : RollScreen ( 1, -1); break;
         case K_CPGDN : RollScreen ( 1,  1); break;


         /*--- draw/erase ---*/
         case K_INS  : iSetMode = (iSetMode==1 ? 0 : 1); break;
         case K_DEL  : iSetMode = (iSetMode==2 ? 0 : 2); break;
         case K_RET  : Set (iX, iY, 1); uITER=0;         break;
         case ' '    : Set (iX, iY, 2); uITER=0;         break;
         case 'G'    : bBreak = TRUE;                    break;
         case 'H'    : Commands ();                      break;
         case 'L'    : LoadObject (posHere); uITER=0;    break;

         case '+':
            GenStep (FALSE);

////////////////////////////////////
//ScrWriteStrAtt ((Repeating () ? "Y" : "N"), 1, YSIZ-1, 0, ATT_THINGIE);
////////////////////////////////////

            break;

         case '-':
            ChangeBuffer (-1);
            break;

         case K_ESC:
            if (bMarking)
               bMarking = FALSE;
            else
               AskQuit ();
            break;

         case K_AC:  /*--- clear ---*/
            if (bMarking)
               ClearObject (posMarkStart, posHere);
            else
               ClearScreen ();
            uITER=0;
            break; 

         case K_AF:  /*--- autofind ---*/
            AutoFind (FALSE);
            break;

         case K_AI:  /*--- LR Mirror ---*/
            c = UserKey (" Mirror Type ? 1=LR 2=LRX 3=TB 4=TBX (1-4 <Esc>)");
            if (c < '1' || c > '4')
               break;
            if (bMarking)
               MirrorObject (posMarkStart, posHere, c - '0', TRUE);
            else
               MirrorObject (pos (0, 0), pos (XSIZ-1, YSIZ-1), c - '0', TRUE);
            uITER=0;
            break;

         case K_AL:  /*--- load ---*/
            PasteBlock ("LAST.DAT", pos (0, 0));
            uITER=0;
            break;

         case K_F11:  /*--- start / end mark ---*/
         case K_AM: 
            if (!bMarking)
               posMarkStart = posHere;
            else
               AskCopyObject (posMarkStart, posHere);
            bMarking = !bMarking;
            break;

         case K_AR:  /*--- random fill ---*/
            if (bMarking)
               {
               RFillObject (posMarkStart, posHere);
               }
            else
               {
               POS pos1, pos2;

               ClearScreen ();
               RandomSize (&pos1, &pos2, Rnd (2));
               RFillObject (pos1, pos2);
               }
            uITER=0;
            break;

         case K_AX:  /*--- exit ---*/
            AskQuit ();
            break;

         default:
            if (c >= K_CF1 && c <= K_CF10)      /*--- paste block    ---*/
               PasteObject (posHere, c - K_CF1);
            
            else if (c >= K_F1 && c <= K_F10)   /*--- save screen    ---*/
               CopyScreen (c - K_F1);
            
            else if (c >= K_SF1 && c <= K_SF10) /*--- restore screen ---*/
               PasteScreen (c - K_SF1);
         }
      if (iSetMode)
         Set (iX, iY, (iSetMode == 1 ? 1 : 0));
      }
   }


/***************************************************************************/
/*                                                                         */
/***************************************************************************/


void Init (void)
   {
	cThingie = MKCELL(THINGIE, ATT_THINGIE);
	cBlank   = MKCELL(BLANK  , ATT_BLANK  );



   ScrInitMetrics ();
   ScrShowCursor (FALSE);
   ScrSetScreenSize (50, 80);

   /*--- alloc as many screen buffers as we can (up to 128) ---*/
   for (SCREEN_BUFFERS=0; SCREEN_BUFFERS<MAXSCR_BUFFERS; SCREEN_BUFFERS++)
      if (!(ppcBuffers[SCREEN_BUFFERS] = calloc (YSIZ, XSIZ)))
         break;

   pcScr = ppcBuffers[iBUFIDX];

   InitFineSleep (600);

   Display (-1, 0, 0, 0);
   srand ((UINT) time (NULL) % 1000);
   }


INT _cdecl main (INT argc, char *argv[])
   {
   Init ();

   KeyAddTrap (K_CF1, NULL);
   KeyAddTrap (K_CF2, NULL);
   KeyAddTrap (K_AF1, AutoRecordTrapFn);
   KeyAddTrap (K_AF2, AutoPlayTrapFn  );

   Commands ();
   AutoFind (FALSE);
   while (TRUE)
      {
      EditMode ();
      CopyBlock ("LAST.DAT", pos (0, 0), pos (XSIZ-1, YSIZ-1));
      GenMode ();
      }
   return 0;
   }


