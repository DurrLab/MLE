/*********************************************************************************************/
/*
 *	File name:		Types.h
 *
 *	Synopsis:		Contains common type definitions.
 *
 *
 *	Taylor Bobrow, Johns Hopkins University (2025)
 *
 */
 /*********************************************************************************************/


#ifndef TYPES_H_
#define TYPES_H_

typedef enum { ODD, EVEN }							IMG_FIELD;

typedef enum { RED, GREEN, BLUE, MONO }				IMG_CHNL;

typedef enum { OFF, WLI, PSE, LSCI,
               MULTI, SSFDI, 
			   WARMUP, SYNC }						MODE;

typedef enum { MAIN, EXTERNAL }						DISPLAY;

#endif /*! TYPES_H_ */