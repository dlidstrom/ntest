// Copyright Chris Welty
//	All Rights Reserved
// This file is distributed subject to GNU GPL version 2. See the files
// Copying.txt and GPL.txt for details.

// evaluator source code
#include "PreCompile.h"
#include "Evaluator.h"
#include "QPosition.h"
#include "Debug.h"
#include "Pos2All.h"
#include "options.h"

float multiplier=10*kStoneValue;

extern CEvaluator* evaluator;

//////////////////////////////////////////////////
// Evaluator base class
//////////////////////////////////////////////////

map<CEvaluatorInfo, CEvaluator*> CEvaluator::evaluatorList;

CEvaluator* CEvaluator::FindEvaluator(char evaluatorType, char coeffSet) {
   CEvaluatorInfo evaluatorInfo;
   CEvaluator* result;
   map<CEvaluatorInfo, CEvaluator*>::iterator ptr;

   evaluatorInfo.evaluatorType=evaluatorType;
   evaluatorInfo.coeffSet=coeffSet;

   ptr=evaluatorList.find(evaluatorInfo);
   if (ptr==evaluatorList.end()) {
      switch(evaluatorType) {
      case 'J': {
         int nFiles= (coeffSet>='9')?10:6;
         result=new CEvaluatorJ(FNBase(evaluatorType, coeffSet), nFiles);
         break;
      }
      default:
         QSSERT(0);
      }
      QSSERT(result);
      evaluatorList[evaluatorInfo]=result;
   }
   else {
      result=(*ptr).second;
   }
   return result;
}

void CEvaluator::Clean() {
   map<CEvaluatorInfo, CEvaluator*>::iterator i;
   for (i=evaluatorList.begin(); i!=evaluatorList.end(); i++)
      delete (*i).second;
}

char* CEvaluator::Filename(char evaluatorType, char coeffSet) {
   sprintf(filenameBuffer, "%scoefficients/%c%c.cof", fnBaseDir, evaluatorType, coeffSet);
   return filenameBuffer;
}

char* CEvaluator::FNBase(char evaluatorType, char coeffSet) {
   sprintf(filenameBuffer, "%scoefficients/%c%c", fnBaseDir, evaluatorType, coeffSet);
   return filenameBuffer;
}

char CEvaluator::filenameBuffer[200];

void CEvaluator::Setup() {
   CQPosition::evaluator=this;
   ::evaluator=this;
}

bool CEvaluator::Pos2Enabled() const {
   return false;
}

//////////////////////////////////////////////////////
// Pattern J evaluator
//	Use 2x4, 2x5, edge+X patterns
//////////////////////////////////////////////////////

CEvaluatorJ::CEvaluatorJ(const char* fnBase, int nFiles) {
   int map,  iFile, coeffStart, mover, packedCoeff, nLen;
   int nIDs, nConfigs, id, config, wconfig, cid, wcid;
   u2 configpm1, configpm2, mapsize;
   bool fHasPotMobs;
   std::vector<float> rawCoeffs;
   TCoeff coeff;
   int iSubset, nSubsets, nEmpty;

   // some parameters are set based on the evaluator version number
   nLen=strlen(fnBase);
   int nSetWidth=60/nFiles;
   char cCoeffSet=fnBase[nLen-1];


   // read in sets
   nSets=0;
   for (iFile=0; iFile<nFiles; iFile++) {
      FILE* fp;
      char fn[100];
      int iVersion;

      // get file name
      strncpy(fn,fnBase,nLen);
      fn[nLen]='a'+(iFile%nFiles);
      strcpy(fn+nLen+1,".cof");

      // open file
      fp=fopen(fn,"rb");
      if (!fp) {
         fprintf(stderr,"Can't open coefficient file %s\n",fn);
         QSSERT(0);
      }

      // read in version and parameter info
      u4 fParams;
      fread(&iVersion, sizeof(iVersion), 1, fp);
      fread(&fParams, sizeof(fParams), 1, fp);
      QSSERT(iVersion==1);

      // calculate the number of subsets
      nSubsets=2;

      for (iSubset=0; iSubset<nSubsets; iSubset++) {
         // allocate memory for the black and white versions of the coefficients
         coeffs[nSets][0].resize(nCoeffsJ);
         coeffs[nSets][1].resize(nCoeffsJ);
         //CHECKNEW(coeffs[nSets][0] && coeffs[nSets][1]);

         // put the coefficients in the proper place
         for (map=0; map<nMapsJ; map++) {

            // inital calculations
            nIDs=mapsJ[map].NIDs();
            nConfigs=mapsJ[map].NConfigs();
            mapsize=mapsJ[map].size;
            coeffStart=coeffStartsJ[map];

            // get raw coefficients from file
            rawCoeffs.resize(nIDs);
            //CHECKNEW(rawCoeffs!=NULL);
            if (fread(&rawCoeffs[0],sizeof(float),nIDs,fp)<nIDs)
               fprintf(stderr, "error reading from file %s\n",fnBase);

            // convert raw coefficients[id] to i2s[config]
            for (config=0; config<nConfigs; config++) {
               id=mapsJ[map].ConfigToID(config);

               coeff = kStoneValue*rawCoeffs[id];
               cid=config+coeffStart;
               //if (rawCoeffs[id]!=0)
               //	printf("%2d %5d %5d %8lf %5d\n",map,id,config,rawCoeffs[id],coeff);

               if (map==PARJ) {
                  // debugging check
                  if (false) {
                     printf("window %d:%d - parity coefficient[%d]=%lf\n",
                            iFile, iSubset, config, rawCoeffs[id]);
                  }
                  // odd-even correction
                  if (cCoeffSet>='A') {
                     if (iFile>=7)
                        coeff+=kStoneValue*.65;
                     else if (iFile==6)
                        coeff+=kStoneValue*.33;
                  }
               }

               // coeff value has first 2 bytes=coeff, 3rd byte=potmob1, 4th byte=potmob2
               if (map<M1J) {	// pattern maps
                  // restrict the coefficient to 2 bytes
                  if (coeff>0x3FFF)
                     packedCoeff=0x3FFF;
                  if (coeff<-0x3FFF)
                     packedCoeff=-0x3FFF;
                  else
                     packedCoeff=coeff;

                  // get linear pot mob info
                  if (map<=D5J) {	// straight-line maps
                     fHasPotMobs=true;
                     configpm1=configToPotMob[0][mapsize][config];
                     configpm2=configToPotMob[1][mapsize][config];
                  }

                  else if (map==C4J) {	// corner triangle maps
                     fHasPotMobs=true;
                     configpm1=configToPotMobTriangle[0][config];
                     configpm2=configToPotMobTriangle[1][config];
                  }

                  else {	// 2x4, 2x5, edge+2X
                     fHasPotMobs=false;
                  }

                  // pack coefficient and pot mob together
                  if (fHasPotMobs)
                     packedCoeff=(packedCoeff<<16) | (configpm1<<8) | (configpm2);

                  // calculate white config
                  wconfig=nConfigs-1-config;
                  wcid=wconfig+coeffStart;
                  coeffs[nSets][0][wcid]=packedCoeff;
                  coeffs[nSets][1][cid]=packedCoeff;
               }

               else {		// non-pattern maps
                  coeffs[nSets][0][cid]=coeff;
                  coeffs[nSets][1][cid]=coeff;
               }
            }
         }

         // fold 2x4 corners into 2x5 corners
         TCoeff* pcf2x4, *pcf2x5;
         TConfig c2x4;

         for(mover=0; mover<2;mover++) {
            pcf2x4=&(coeffs[nSets][mover][coeffStartsJ[C2x4J]]);
            pcf2x5=&(coeffs[nSets][mover][coeffStartsJ[C2x5J]]);
            // fold coefficients in
            for (config=0; config<9*6561; config++) {
               c2x4=configs2x5To2x4[config];
               pcf2x5[config]+=pcf2x4[c2x4];
            }
            // zero out 2x4 coefficients
            for (config=0;config<6561; config++) {
               pcf2x4[config]=0;
            }
         }

         // Set the pcoeffs array and the fParameters
         for (nEmpty=59-nSetWidth*iFile; nEmpty>=50-nSetWidth*iFile; nEmpty--) {
            // if this is a set of the wrong parity, do nothing
            if ((nEmpty&1)==iSubset)
               continue;
            for (mover=0; mover<2; mover++) {
               pcoeffs[nEmpty][mover]=coeffs[nSets][mover];
            }
         }

         nSets++;
      }
      fclose(fp);
   }
}

// pos2 evaluators
CValue CEvaluatorJ::EvalMobs(u4 nMovesPlayer, u4 nMovesOpponent) const {
   return ValueJMobs(pcoeffs[nEmpty_], nMovesPlayer, nMovesOpponent);
}

bool CEvaluatorJ::Pos2Enabled() const {
   return true;
}
