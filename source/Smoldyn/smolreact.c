/* Steven Andrews, started 10/22/2001.
 This is a library of functions for the Smoldyn program.  See documentation
 called Smoldyn_doc1.pdf and Smoldyn_doc2.pdf.
 Copyright 2003-2011 by Steven Andrews.  This work is distributed under the terms
 of the Gnu General Public License (GPL). */

#include <stdio.h>
#include <math.h>
#include <ctype.h>
#include <string.h>
#include <float.h>
#include "math2.h"
#include "math.h"
#include "random2.h"
#include "Rn.h"
#include "rxnparam.h"
#include "smoldyn.h"
#include "smoldynfuncs.h"
#include "Sphere.h"
#include "Zn.h"
#include "Geometry.h"
#include <cstdint>
#include <iostream>
#include <algorithm>

#include "smoldynconfigure.h"

using namespace std;
/******************************************************************************/
/********************************** Reactions *********************************/
/******************************************************************************/


/******************************************************************************/
/****************************** Local declarations ****************************/
/******************************************************************************/

// enumerated types
char *rxnrp2string(enum RevParam rp,char *string);

// low level utilities
int rxnpackident(int order,int maxspecies,int *ident);
void rxnunpackident(int order,int maxspecies,int ipack,int *ident);
GSList* bireact_test(simptr sim, int order, moleculeptr mptr1, moleculeptr mptr2,int *len, double *dc1, double *dc2, double *bindrad2);
void posptr_assign(molssptr mols, moleculeptr mptr, moleculeptr mptr_bind, int site);

enum MolecState rxnpackstate(int order,enum MolecState *mstate);
void rxnunpackstate(int order,enum MolecState mspack,enum MolecState *mstate);
int rxnreactantstate(rxnptr rxn,enum MolecState *mstate,int convertb2f);
int rxnallstates(rxnptr rxn);
gpointer findreverserxn(rxnssptr rxnss,gpointer rxnptr,moleculeptr mptr1, moleculeptr mptr2);

// memory management
rxnptr rxnalloc(int order);
void rxnfree(rxnptr rxn);
rxnssptr rxnssalloc(rxnssptr rxnss,int molec_num,int maxspecies,int maxsitecode);

// data structure output

// parameter calculations
int rxnsetrates(simptr sim,int order,char *erstr);
int rxnsetproduct(simptr sim,int order,int r,char *erstr);
int rxnsetproducts(simptr sim,int order,char *erstr);
double rxncalcrate(simptr sim,int order,int r,double *pgemptr);
void rxncalctau(simptr sim,int molec_num);

// structure set up
rxnssptr rxnreadstring(simptr sim,ParseFilePtr pfp,rxnssptr rxnss,char *word,char *line2);
int rxnsupdateparams(simptr sim);
int rxnsupdatelists(simptr sim,int molec_num);

// core simulation functions
int morebireact(rxnssptr rxnss,gpointer rptr,moleculeptr mptr1,moleculeptr mptr2,int ll1,int m1,int ll2,enum EventType et,double *vect,int rxn_site_indx1,int rxn_site_indx2,double radius,double dc1,double dc2);

// rxn input process
int rxncond_parse(molssptr mols, char *cond, int rct_ident, int **sites_state_ptr, int *sites_num, int **sites_indx);
// int rxncond_eval(molssptr mols, moleculeptr mptr, char **rxncond);
void prdsites_parse(molssptr mols, char *sites_state_str, char *molec_num, prd_mptr prd);
int rxnlhs(molssptr mols, char *molec1, rct_mptr rct);
double rxnrhs(molssptr mols, char *molec1, char *molec2, prd_mptr prd1, prd_mptr prd2);
void rxnsetup(simptr sim, rxnssptr rxnss, rxnptr rxn, char *rname, int order, int molec_num, int nprod, rct_mptr rct1, rct_mptr rct2, prd_mptr prd1, prd_mptr prd2, compartptr cmpt, surfaceptr srf, double flt1);

double radius(simptr sim, gpointer rptr, moleculeptr mptr1, moleculeptr mptr2, double *dc1, double *dc2, int molec_gen);

/******************************************************************************/
/********************************* enumerated types ***************************/
/******************************************************************************/

/* rxnstring2rp */
enum RevParam rxnstring2rp(char *string) {
	enum RevParam ans;

	if(!strcmp(string,"i")) ans=RPirrev;
	else if(!strcmp(string,"a")) ans=RPconfspread;
	else if(!strcmp(string,"p")) ans=RPpgem;
	else if(!strcmp(string,"x")) ans=RPpgemmax;
	else if(!strcmp(string,"r")) ans=RPratio;
	else if(!strcmp(string,"b")) ans=RPunbindrad;
	else if(!strcmp(string,"q")) ans=RPpgem2;
	else if(!strcmp(string,"y")) ans=RPpgemmax2;
	else if(!strcmp(string,"s")) ans=RPratio2;
	else if(!strcmp(string,"o")) ans=RPoffset;
	else if(!strcmp(string,"f")) ans=RPfixed;
	else if(!strcmp(string,"irrev")) ans=RPirrev;
	else if(!strcmp(string,"confspread")) ans=RPconfspread;
	else if(!strcmp(string,"bounce")) ans=RPbounce;
	else if(!strcmp(string,"pgem")) ans=RPpgem;
	else if(!strcmp(string,"pgemmax")) ans=RPpgemmax;
	else if(!strcmp(string,"ratio")) ans=RPratio;
	else if(!strcmp(string,"unbindrad")) ans=RPunbindrad;
	else if(!strcmp(string,"pgem2")) ans=RPpgem2;
	else if(!strcmp(string,"pgemmax2")) ans=RPpgemmax2;
	else if(!strcmp(string,"ratio2")) ans=RPratio2;
	else if(!strcmp(string,"offset")) ans=RPoffset;
	else if(!strcmp(string,"fixed")) ans=RPfixed;
	else ans=RPnone;
	return ans;
}


/* rxnrp2string */
char *rxnrp2string(enum RevParam rp,char *string) {
	if(rp==RPirrev) strcpy(string,"irrev");
	else if(rp==RPconfspread) strcpy(string,"confspread");
	else if(rp==RPbounce) strcpy(string,"bounce");
	else if(rp==RPpgem) strcpy(string,"pgem");
	else if(rp==RPpgemmax) strcpy(string,"pgemmax");
	else if(rp==RPpgemmaxw) strcpy(string,"pgemmaxw");
	else if(rp==RPratio) strcpy(string,"ratio");
	else if(rp==RPunbindrad) strcpy(string,"unbindrad");
	else if(rp==RPpgem2) strcpy(string,"pgem2");
	else if(rp==RPpgemmax2) strcpy(string,"pgemmax2");
	else if(rp==RPratio2) strcpy(string,"ratio2");
	else if(rp==RPoffset) strcpy(string,"offset");
	else if(rp==RPfixed) strcpy(string,"fixed");
	else strcpy(string,"none");
	return string; }


/******************************************************************************/
/***************************** low level utilities ****************************/
/******************************************************************************/

/* readrxnname. */
int readrxnname(simptr sim,char *rname,int *molecnum_ptr,rxnptr *rxnpt) {
	int k,r;

	r=-1;
	for(k=0;k<MAXORDER && r==-1;k++)
		if(sim->rxnss[k])
			r=stringfind(sim->rxnss[k]->rname,sim->rxnss[k]->totrxn,rname);
	k--;
	if(r>=0) {
		if(molecnum_ptr) *molecnum_ptr=sim->rxnss[k]->molec_num;
		if(rxnpt) *rxnpt=sim->rxnss[k]->rxn[r]; }
	return r; }

/* rxnpackident */
int rxnpackident(int order,int maxspecies,int *ident) {
	if(order==0) return 0;
	if(order==1) return ident[0];
	if(order==2) return ident[0]*maxspecies+ident[1];
	return 0; }


/* rxnunpackident */
void rxnunpackident(int order,int maxspecies,int ipack,int *ident) {
	if(order==0);
	else if(order==1) ident[0]=ipack;
	else if(order==2) {ident[0]=ipack/maxspecies;ident[1]=ipack%maxspecies;}
	return; }


/* rxnpackstate */
enum MolecState rxnpackstate(int order,enum MolecState *mstate) {
	if(order==0) return (MolecState)0;
	if(order==1) return mstate[0];
	if(order==2) return (MolecState)(mstate[0]*MSMAX1+mstate[1]);
	return (MolecState)0;
}

/* rxnunpackstate */
void rxnunpackstate(int order,enum MolecState mspack,enum MolecState *mstate) {
	if(order==0);
	else if(order==1) mstate[0]=mspack;
	else if(order==2) { mstate[0]=(MolecState)(mspack/MSMAX1);mstate[1]=(MolecState)(mspack%MSMAX1); }
	return;
}

/* rxnallstates */
int rxnallstates(rxnptr rxn) {
 	enum MolecState ms;
	int nms2o;

	// if(rxn->rxnss->order==0) return 0;
	if(rxn->rxnss->molec_num==0) return 0;
	nms2o=intpower(MSMAX1,rxn->rxnss->molec_num);
	for(ms=(MolecState)0;ms<nms2o && rxn->permit[ms];ms=(MolecState)(ms+1));
	if(ms==nms2o) return 1;
	return 0; }


/* rxnreactantstate */
int rxnreactantstate(rxnptr rxn,enum MolecState *mstate,int convertb2f) {
	int order,permit;
	enum MolecState ms,ms1,ms2;
	int mspair;

	order=rxn->order;
	permit=0;

	if(order==0) permit=1;

	else if(order==1) {
		if(rxn->permit[MSsoln]) {
			ms=MSsoln;
			permit=1; }
		else if(rxn->permit[MSbsoln]) {
			ms=MSbsoln;
			permit=1; }
		else {
			for(ms=(MolecState)0;ms<MSMAX1 && !rxn->permit[ms];ms=(MolecState)(ms+1));
			if(ms<MSMAX1) permit=1; }
		if(permit && convertb2f && ms==MSbsoln) ms=MSsoln;
		if(mstate) {
			if(permit) mstate[0]=ms;
			else mstate[0]=MSnone; }}

	else if(order==2) {
		if(rxn->permit[MSsoln*MSMAX1+MSsoln]) {
			ms1=ms2=MSsoln;
			permit=1; }
		else if(rxn->permit[MSsoln*MSMAX1+MSbsoln]) {
			ms1=MSsoln;
			ms2=MSbsoln;
			permit=1; }
		else if(rxn->permit[MSbsoln*MSMAX1+MSsoln]) {
			ms1=MSbsoln;
			ms2=MSsoln;
			permit=1; }
		else if(rxn->permit[MSbsoln*MSMAX1+MSbsoln]) {
			ms1=MSbsoln;
			ms2=MSbsoln;
			permit=1; }
		if(!permit) {
			for(ms1=(MolecState)0;ms1<MSMAX1 && !rxn->permit[ms1*MSMAX1+MSsoln];ms1=(MolecState)(ms1+1));
			if(ms1<MSMAX1) {
				ms2=MSsoln;
				permit=1; }}
		if(!permit) {
			for(ms2=(MolecState)0;ms2<MSMAX1 && !rxn->permit[MSsoln*MSMAX1+ms2];ms2=(MolecState)(ms2+1));
			if(ms2<MSMAX1) {
				ms1=MSsoln;
				permit=1; }}
		if(!permit) {
			for(mspair=0;mspair<MSMAX1*MSMAX1 && !rxn->permit[mspair];mspair++);
			if(mspair<MSMAX1*MSMAX1) {
				ms1=(MolecState)(mspair/MSMAX1);
				ms2=(MolecState)(mspair%MSMAX1);
				permit=1; }}
		if(permit && convertb2f) {
			if(ms1==MSbsoln) ms1=MSsoln;
			if(ms2==MSbsoln) ms2=MSsoln; }
		if(mstate) {
			mstate[0]=permit?ms1:MSnone;
			mstate[1]=permit?ms2:MSnone; }}

	return permit; }


/* findreverserxn */
gpointer findreverserxn(rxnssptr rxnss,gpointer rptr,moleculeptr mptr1,moleculeptr mptr2) {
	rxnssptr rxnssr;
	rxnptr rxn,rxnr;
	int j,k,s,sites_val1,sites_val2,entry,entry1,entry2,l;
	GSList *r;
	double dc1,dc2,dsum,rate3,unbindrad,rpar;
	intptr_t r_indx,len_t;
	simptr sim;
	moleculeptr mptrA, mptrB; 
	gpointer rev;
	
	rev=g_hash_table_lookup(rxnss->rxnr_ptr,rptr);
	if(rev) return rev;
	else{
		sim=rxnss->sim;
		rxn=rxnss->rxn[(int)(intptr_t)((GSList*)rptr)->data];
		if(rxn->molec_num==1){
			rxnssr=sim->rxnss[1];
			sites_val1=mptr1->sites_val;
			for(s=0;s<rxn->prd[0]->sites_num;s++) {
				sites_val1-=mptr1->sites[rxn->prd[0]->sites_indx[s]]->value[0];
				sites_val1+=rxn->prd[0]->sites_val[s];
			}
			entry=g_pairing(mptr1->ident,sites_val1);
			r=(GSList*)g_hash_table_lookup(rxnssr->table,GINT_TO_POINTER(entry));	

			if(r){
				len_t=(intptr_t)g_hash_table_lookup(rxnssr->entrylist,r);
				for(l=0;l<(int)len_t;l++,r=r->next){
					r_indx=(intptr_t)r->data;
					rxnr=rxnssr->rxn[(int)r_indx];
					for(k=0;k<rxnr->rct[0]->states_num;k++){
						if(rxnr->rct[0]->states[k]==sites_val1){
							rev=r;
							break;	
					}}
					if(rev) break;
			}}
			else { rev=NULL;};
		}
	
		if(rxn->molec_num==2 && rxn->nprod>0){
			if(!mptr2) return 0;
			rxnssr=sim->rxnss[2];	
			if(rxn->rct[0]->ident==mptr1->ident && rxn->rct[1]->ident==mptr2->ident) { mptrA=mptr1; mptrB=mptr2; }
			else if(rxn->rct[1]->ident==mptr1->ident && rxn->rct[0]->ident==mptr2->ident) { mptrA=mptr2; mptrB=mptr1; }
			else {rev=NULL; return rev;}

			sites_val1=mptrA->sites_val;
			sites_val2=mptrB->sites_val;
			for(s=0;s<rxn->prd[0]->sites_num;s++){
				sites_val1-=power(2,rxn->prd[0]->sites_indx[s])*mptrA->sites[rxn->prd[0]->sites_indx[s]]->value[0];
				sites_val1+=power(2,rxn->prd[0]->sites_indx[s])*rxn->prd[0]->sites_val[s];
			}

			if(rxn->prd[1]){
				for(s=0;s<rxn->prd[1]->sites_num;s++){
					sites_val2-=power(2,rxn->prd[1]->sites_indx[s])*mptrB->sites[rxn->prd[1]->sites_indx[s]]->value[0];
					sites_val2+=power(2,rxn->prd[1]->sites_indx[s])*rxn->prd[1]->sites_val[s];
				}
			}
			entry1=g_pairing(mptrA->ident,sites_val1);
			entry2=g_pairing(mptrB->ident,sites_val2);	
			entry=g_pairing(entry1,entry2);
			r=(GSList*)g_hash_table_lookup(rxnssr->table,GINT_TO_POINTER(entry));
		
			if(!r){
				entry=g_pairing(entry2,entry1);
				r=(GSList*)g_hash_table_lookup(rxnssr->table,GINT_TO_POINTER(entry));
			}
			if(r){
				len_t=(intptr_t)g_hash_table_lookup(rxnssr->entrylist,r);		
				for(l=0;l<(int)len_t;l++,r=r->next){							
					r_indx=(intptr_t)r->data;
					rxnr=rxnssr->rxn[(int)r_indx];
					if(rxn->order==rxnr->nprod && rxn->nprod==rxnr->order){
						/*
						for(s=0;s<rxnr->prd[0]->sites_num;s++){
							if(rxnr->prd[0]->sites_indx[s]!=rxn->prd[0]->sites_indx[s]||rxnr->prd[0]->sites_val[s]!=1-rxn->prd[0]->sites_val[s])
							 	goto next_rxn;
						}
						for(s=0;s<rxnr->prd[1]->sites_num;s++){
							if(rxnr->prd[1]->sites_indx[s]!=rxn->prd[1]->sites_indx[s]||rxnr->prd[1]->sites_val[s]!=1-rxn->prd[1]->sites_val[s])
								goto next_rxn;
						}
						*/
						rev=r;
						printf("rxn:%s rxnr:%s\n", rxn->rname, rxnr->rname);
						break;		
					}
					next_rxn: continue;
			}}
			else { rev=NULL;}
		}
		else rev=NULL;
	
		if(rev){
			g_hash_table_insert(rxnss->rxnr_ptr,rptr,rev);
			g_hash_table_insert(rxnssr->rxnr_ptr,rev,rptr);
			if(rxnr->rparamt==RPpgem && rxn->rparamt!=RPpgem){
				rxn->rparam=rxnr->rparam;
				rxn->rparamt=RPpgem;
			}
			else if(rxn->rparamt==RPpgem && rxnr->rparamt!=RPpgem){
				rxnr->rparam=rxn->rparam;
				rxnr->rparamt=RPpgem;
			}
			else if(rxn->rparamt==RPnone || rxnr->rparamt==RPnone){
				rxnr->rparamt=RPpgemmax;
				rxnr->rparam=0.2;
				rxn->rparamt=RPpgemmax;
				rxn->rparam=0.2;
			}
			if(rxn->order==2){
				rxn->k_on=rxnr->k_on=rxn->rate;
				rxn->k_off=rxnr->k_off=rxnr->rate;
			}
			else {
				rxn->k_on=rxnr->k_on=rxnr->rate;
				rxn->k_off=rxnr->k_off=rxn->rate;
			}
		}
	}	
	return rev; 
}


/* rxnisprod */
int rxnisprod(simptr sim,int i,enum MolecState ms,int code) {
	int k,r,prd;
	rxnssptr rxnss;
	rxnptr rxn;

	for(k=0;k<MAXORDER;k++){
		rxnss=sim->rxnss[k];
		if(rxnss) {
			for(r=0;r<rxnss->totrxn;r++) {
				rxn=rxnss->rxn[r];
				if(rxn){
					for(prd=0;prd<rxn->nprod;prd++)
						if(rxn->prd[prd]->ident==i && rxn->prd[prd]->prdstate==ms) {
							if(code==0) return 1;
							// if(rxn->rparamt==RPconfspread || rxn->unbindrad>0) return 1;
							if(rxn->rparamt==RPconfspread) return 1;
							if(dotVVD(rxn->prdpos[prd],rxn->prdpos[prd],sim->dim)>0) return 1; }}}}}
	return 0; }


/******************************************************************************/
/****************************** memory management *****************************/
/******************************************************************************/

/* rxnalloc */
rxnptr rxnalloc(int order) {
	rxnptr rxn;
	int nms2o;
	enum MolecState ms;
	
	CHECKMEM(rxn=(rxnptr) malloc(sizeof(struct rxnstruct)));
	rxn->rxnss=NULL;
	rxn->rname=NULL;
	rxn->permit=NULL;
	rxn->nprod=0;
	rxn->molec_num=0;
	rxn->prdserno=NULL;
	rxn->logserno=NULL;
	rxn->logfile=NULL;
	rxn->rate=-1;
	rxn->rct=NULL;
	rxn->prd=NULL;

	rxn->prob=-1;
	rxn->tau=-1;
	rxn->rparamt=RPnone;
	rxn->rparam=0;
	rxn->prdpos=NULL;
	rxn->disable=0;
	rxn->cmpt=NULL;
	rxn->srf=NULL;
	// rxn->radius=g_hash_table_new(g_direct_hash, g_direct_equal);
	// rxn->radius=radius_map();
	//rxn->radius=NULL;
	rxn->bindrad=-1;
	rxn->unbindrad=-1;
	rxn->k_on=-1;
	rxn->k_off=-1;
	rxn->phi=-1;
	rxn->miu=-1;

	if(order>0) {
		nms2o=intpower(MSMAX1,order);
		CHECKMEM(rxn->permit=(int*)calloc(nms2o,sizeof(int)));
		for(ms=(MolecState)0;ms<nms2o;ms=(MolecState)(ms+1)) rxn->permit[ms]=0; 
	}
	return rxn;

 failure:
	rxnfree(rxn);
	simLog(NULL,10,"Unable to allocate memory in rxnalloc");
	return NULL; }


/* rxnfree */
void rxnfree(rxnptr rxn) {
	int k,prd, cond;

	rxn->rxnss=NULL;
	if(!rxn) return;
	if(rxn->prdpos){
		for(prd=0;prd<rxn->nprod;prd++){ 
			if(rxn->prdpos[prd]) free(rxn->prdpos[prd]);		
		}
		rxn->prdpos=NULL; 			
	}
	if(rxn->rct){
		for(k=0;k<rxn->order;k++){
			free(rxn->rct[k]->states);
			free(rxn->rct[k]->sites_indx);
		}
		free(rxn->rct);
	}
	if(rxn->prd){
		for(k=0;k<rxn->nprod;k++){
			free(rxn->prd[k]->sites_indx);
			free(rxn->prd[k]->sites_val);
		}
		free(rxn->prd);
	}

	if(rxn->prdserno)  free(rxn->prdserno);		rxn->prdserno=NULL;

	List_FreeLI(rxn->logserno);
	// if(rxn->logfile)	free(rxn->logfile);		rxn->logfile=NULL;		// printf("rxn->logfile\n");
	if(rxn->permit){
		free(rxn->permit);		
		rxn->permit=NULL;
	}

	if(rxn->rname)	free(rxn->rname);	rxn->rname=NULL;
	if(rxn){
		free(rxn); 	
		rxn=NULL; 	// printf("rxn\n");
	}
	return; }


/* rxnssalloc */
rxnssptr rxnssalloc(rxnssptr rxnss,int molec_num,int maxspecies, int maxsitecode) {	
	int i,i2,failfree;
	int *newnrxn,**newtable,**newbinding;
	int j,sz, species_sz, sitecode_sz, tbl2_sz;

	failfree=0;
	if(!rxnss) {															// new reaction superstructure
		rxnss=(rxnssptr) malloc(sizeof(struct rxnsuperstruct));
		CHECKMEM(rxnss);
		failfree=1;
		rxnss->condition=SCinit;
		rxnss->sim=NULL;
		rxnss->molec_num=molec_num;
		rxnss->maxspecies=0;
		rxnss->maxsitecode=0;
		rxnss->maxlist=0;
		rxnss->nrxn=NULL;
		rxnss->table=NULL;
		rxnss->maxrxn=0;
		rxnss->totrxn=0;
		rxnss->rname=NULL;
		rxnss->rxn=NULL;
		// rxnss->rxnmollist=NULL;
		rxnss->table=g_hash_table_new(g_direct_hash,g_direct_equal);
		// rxnss->entrylist=NULL;
		rxnss->entrylist=g_hash_table_new(g_direct_hash,g_direct_equal);
		//rxnss->adjusted_kon=g_hash_table_new(g_direct_hash,g_direct_equal);
		rxnss->rxnaff=g_hash_table_new(g_direct_hash,g_direct_equal);
		rxnss->bindrad_eff=g_hash_table_new(g_direct_hash,g_direct_equal);
		rxnss->probrng_l=g_hash_table_new(g_direct_hash,g_direct_equal);
		rxnss->probrng_h=g_hash_table_new(g_direct_hash,g_direct_equal);
		//rxnss->rmaps=g_hash_table_new(g_direct_hash,g_direct_equal);
		rxnss->rxnr_ptr=g_hash_table_new(g_direct_hash,g_direct_equal);
		rxnss->rxn_ord1st=g_hash_table_new(g_direct_hash,g_direct_equal);
		rxnss->binding=NULL;
		rxnss->radius=g_hash_table_new(g_direct_hash,g_direct_equal);
	 }

	if(maxspecies>rxnss->maxspecies || maxsitecode>rxnss->maxsitecode) {		// initialize or expand nrxn and table
		rxnss->maxspecies=maxspecies;
		rxnss->maxsitecode=maxsitecode;
		if(molec_num==2){
			rxnss->binding=(int**)calloc(maxspecies,sizeof(int*));
			for(i=0;i<maxspecies;i++){
				rxnss->binding[i]=(int*)calloc(maxspecies,sizeof(int));
				for(j=0;j<maxspecies;j++)
					rxnss->binding[i][j]=0;
	}}} 

	return rxnss;

 failure:
	if(failfree) rxnssfree(rxnss);
	simLog(NULL,10,"Unable to allocate memory in rxnssalloc");
	return NULL;
}


/* rxnssfree */
void rxnssfree(rxnssptr rxnss) {
	int r,i,tbl_sz,sz1,sz2,tbl2_sz;
	rxnptr rxn_tmp, next_tmp;

	if(!rxnss) return;
	// if(rxnss->rxnmollist) free(rxnss->rxnmollist);
	if(rxnss->rxn){
		for(r=0;r<rxnss->maxrxn;r++) {
			if(rxnss->rxn[r])
				rxnfree(rxnss->rxn[r]);
		}
	}	

	free(rxnss->rxn);	rxnss->rxn=NULL;
	if(rxnss->rname)
		for(r=0;r<rxnss->maxrxn;r++) free(rxnss->rname[r]);
	free(rxnss->rname);

	if(rxnss->table) g_hash_table_destroy(rxnss->table);	
	if(rxnss->entrylist) 
		g_hash_table_destroy(rxnss->entrylist);
	/*
	if(rxnss->adjusted_kon)
		g_hash_table_destroy(rxnss->adjusted_kon);
	*/
	if(rxnss->rxnaff)
		g_hash_table_destroy(rxnss->rxnaff);
	if(rxnss->bindrad_eff)
		g_hash_table_destroy(rxnss->bindrad_eff);
	
	if(rxnss->probrng_l)
		g_hash_table_destroy(rxnss->probrng_l);
	if(rxnss->probrng_h)	
		g_hash_table_destroy(rxnss->probrng_h);

	//if(rxnss->rmaps)
	//	g_hash_table_destroy(rxnss->rmaps);

	if(rxnss->rxnr_ptr)
		g_hash_table_destroy(rxnss->rxnr_ptr);
	if(rxnss->rxn_ord1st)
		g_hash_table_destroy(rxnss->rxn_ord1st);
	if(rxnss->radius)
		g_hash_table_destroy(rxnss->radius);

	if(rxnss->binding){
		for(i=0;i<rxnss->maxspecies;i++) free(rxnss->binding[i]);	
		free(rxnss->binding);
	}
	
	free(rxnss->nrxn);

	free(rxnss);
	return; }


/* rxnexpandmaxspecies */
int rxnexpandmaxspecies(simptr sim,int maxspecies,int maxsitecode) {
	rxnssptr rxnss;
	int k;

	for(k=0;k<MAXORDER;k++) {
			if(sim->rxnss[k]){ 
				if(sim->rxnss[k]->maxspecies<maxspecies) {
					rxnss=sim->rxnss[k];
					rxnss=rxnssalloc(rxnss,rxnss->molec_num,maxspecies,maxsitecode);
					if(!rxnss) return k+1; }}}
	return 0; }


/******************************************************************************/
/**************************** data structure output ***************************/
/******************************************************************************/


/* rxnoutput */
void rxnoutput(simptr sim,int molec_num) {
	rxnssptr rxnss;
	int d,dim,ord;
	// int maxlist,maxll2o,ll,ni2o,identlist[MAXORDER]; 
	int i,j,r,s,rct,prd,rev,orderr,rr,i1,i2,o2,r2;
	long int serno;
	rxnptr rxn,rxnr;
	enum MolecState ms,ms1,ms2,nms2o,statelist[MAXORDER];
	double dsum,step,pgem,rate3,bindrad,rparam,ratio;
	char string[STRCHAR];
	enum RevParam rparamt;
	int site, order;

	if(!sim || !sim->mols || !sim->rxnss[molec_num]) {
		simLog(sim,2," No reactions of molec_num %i\n\n",molec_num);
		return; }
	rxnss=sim->rxnss[molec_num];
	if(rxnss->molec_num!=molec_num) return;
	if(rxnss->condition!=SCok)
		simLog(sim,2," structure condition: %s\n",simsc2string(rxnss->condition,string));
	dim=sim->dim;

	simLog(sim,1," allocated for %i species\n",rxnss->maxspecies-1);
	simLog(sim,1," allocated for %i molecule lists\n",rxnss->maxlist);

	simLog(sim,2," %i reactions defined",rxnss->totrxn);
	simLog(sim,1,", of %i allocated",rxnss->maxrxn);
	simLog(sim,2,"\n");

	simLog(sim,2," Reaction details:\n");
	for(r=0;r<rxnss->totrxn;r++) {
		rxn=rxnss->rxn[r];
		order=rxn->order;
		if(!rxn) continue;
		simLog(sim,2,"  Reaction %s:",rxn->rname);
		if(order==0) simLog(sim,2," 0");							// reactants
		for(rct=0;rct<molec_num;rct++) {
			simLog(sim,2," %s",sim->mols->spname[rxn->rct[rct]->ident]);
			if(rct<molec_num-1 && order==1) simLog(sim,2,"~");
			if(rct<molec_num-1 && order==2) simLog(sim,2,"+"); 
			if(rxn->rct[rct]->rctstate!=MSsoln) simLog(sim,2," (%s)",molms2string(rxn->rct[rct]->rctstate,string));
		}
		simLog(sim,2," ->");
		if(rxn->nprod==0) simLog(sim,2," 0");						// products
		for(prd=0;prd< rxn->nprod;prd++) {
			simLog(sim,2," %s",sim->mols->spname[rxn->prd[prd]->ident]);
			if(rxn->prd[prd]->prdstate!=MSsoln) simLog(sim,2," (%s)",molms2string(rxn->prd[prd]->prdstate,string));
			if(prd<rxn->nprod-1 && order==1) simLog(sim,2," +"); 
			if(prd<rxn->nprod-1 && order==2) simLog(sim,2,"~");
		}
		simLog(sim,2,"\n");

		for(rct=0;rct<molec_num;rct++)								// permit
			if(rxn->rct[rct]->rctstate==MSsome) rct=order+1;
		if(rct==order+1) {
			simLog(sim,2,"   permit:");
			nms2o=(MolecState)intpower(MSMAX1,order);
			for(ms=(MolecState)0;ms<nms2o;ms=(MolecState)(ms+1)) {
				if(rxn->permit[ms]) {
					rxnunpackstate(order,ms,statelist);
					simLog(sim,2," ");
					for(ord=0;ord<order;ord++)
						simLog(sim,2,"%s%s",molms2string(statelist[ms],string),ord<order-1?"+":""); }}}

		if(rxn->prdserno) {											// product serial number rules
			simLog(sim,2,"   serial number rules:");
			for(prd=0;prd<rxn->nprod;prd++) {
				serno=rxn->prdserno[prd];
				if(serno==0) simLog(sim,2," new");
				else if(serno==-1) simLog(sim,2," r1");
				else if(serno==-2) simLog(sim,2," r2");
				else if(serno<=-10) simLog(sim,2," p%i",(int)(-9-serno));
				else simLog(sim,2," %li",serno); }
			simLog(sim,2,"\n"); }

		if(rxn->logserno) {											// reaction log
			simLog(sim,2,"   reaction logged to file %s",rxn->logfile);
			if(rxn->logserno->n==0)
				simLog(sim,2," for all molecules\n");
			else {
				simLog(sim,2,"\n    molecules:");
				for(i=0;i<rxn->logserno->n && i<5;i++)
					simLog(sim,2," %li",rxn->logserno->xs[i]);
				if(i<rxn->logserno->n)
					simLog(sim,2,"...%li",rxn->logserno->xs[rxn->logserno->n-1]);
				simLog(sim,2,"\n"); }}

		if(rxn->cmpt) simLog(sim,2,"   compartment: %s\n",rxn->cmpt->cname);
		if(rxn->srf) simLog(sim,2,"   surface: %s\n",rxn->srf->sname);
		// if(rxn->rate>=0) simLog(sim,2,"   requested and actual rate constants: %g, %g\n",rxn->rate,rxncalcrate(sim,order,r,&pgem));
		// else simLog(sim,2,"   actual rate constant: %g\n",rxncalcrate(sim,order,r,&pgem));
		if(pgem>=0) simLog(sim,2,"   geminate recombination probability: %g\n",pgem);
		if(rxn->rparamt==RPconfspread) simLog(sim,2,"  conformational spread reaction\n");
		if(rxn->tau>=0) simLog(sim,2,"   characteristic time: %g\n",rxn->tau);
		if(order==0) simLog(sim,2,"   average reactions per time step: %g\n",rxn->prob);
		else if(order==1) simLog(sim,2,"  conditional reaction probability per time step: %g\n",rxn->prob);			// this is the conditional probability, with the condition that prior possible reactions did not happen
		else if(rxn->prob>=0 && rxn->prob!=1) simLog(sim,2,"   reaction probability after collision: %g\n",rxn->prob);
		// if(rxn->bindrad2>=0) simLog(sim,2,"   binding radius: %g\n",sqrt(rxn->bindrad2));
		
		/*
		if(rxn->nprod==2) {						// then order=1, molec_num=1							
			if(rxn->unbindrad>=0) simLog(sim,2,"   unbinding radius: %g\n",rxn->unbindrad);
			else if(rxn->rparamt==RPbounce) simLog(sim,2,"   unbinding radius: calculated from molecule overlap\n");
			else simLog(sim,2,"   unbinding radius: 0\n");
			
			if(rxn->rparamt!=RPconfspread && rxn->rparamt!=RPbounce) && findreverserxn(sim,order,molec_num,r,&o2,&r2)==1){
				rxnr=sim->rxnss[o2]->rxn[r2];			// molec_num=2
				dsum=MolCalcDifcSum(sim,rxn->ident[0],rxn->prdstate[0],rxn->ident[1],rxn->prdstate[1]);
				rate3=rxncalcrate(sim,o2,2,r2,NULL);
				if(rxn->ident[0]==rxn->ident[1]) rate3*=2;
				rparamt=rxn->rparamt;
				rparam=rxn->rparam;
				if(rparamt==RPunbindrad) bindrad=bindingradius(rate3,0,dsum,rparam,0);
				else if(rparamt==RPratio) bindrad=bindingradius(rate3,0,dsum,rparam,1);
				else if(rparamt==RPpgem || rparamt==RPpgemmax || rparamt==RPpgemmaxw) {
					bindrad=bindingradius(rate3*(1.0-rparam),0,dsum,-1,0);
					pgem=rparam; }
				else bindrad=bindingradius(rate3,0,dsum,-1,0);
				simLog(sim,2,"   unbinding radius if dt were 0: %g\n",unbindingradius(pgem,0,dsum,bindrad)); }
			
		}
		*/
		/*
		// binding reactions
		if(order==2 && rxnreactantstate(rxn,statelist,1)) {
			ms1=statelist[0];
			ms2=statelist[1];
			i1=rxn->rct[0]->ident;
			i2=rxn->rct[1]->ident;
			dsum=MolCalcDifcSum(sim,i1,ms1,i2,ms2);
			// rev=findreverserxn(sim,order,2,r,&o2,&r2);
			rev=0;
			rate3=rxncalcrate(sim,order,r,NULL);
			if(i1==i2) rate3*=2;											// same reactants
			if(rev==1) {
				rparamt=sim->rxnss[o2]->rxn[r2]->rparamt;					// molec_num=2
				rparam=sim->rxnss[o2]->rxn[r2]->rparam; 
			}
			else {
				rparamt=RPnone;
				rparam=0; 	}
			if(rparamt==RPconfspread) bindrad=-1;
			else if(rparamt==RPbounce) bindrad=-1;
			else if(rparamt==RPunbindrad) bindrad=bindingradius(rate3,0,dsum,rparam,0);
			else if(rparamt==RPratio) bindrad=bindingradius(rate3,0,dsum,rparam,1);
			else if(rparamt==RPpgem || rparamt==RPpgemmax || rparamt==RPpgemmaxw) bindrad=bindingradius(rate3*(1.0-rparam),0,dsum,-1,0);
			else bindrad=bindingradius(rate3,0,dsum,-1,0);
			if(bindrad>=0) simLog(sim,2,"   binding radius if dt were 0: %g\n",bindrad);
			step=sqrt(2.0*dsum*sim->dt);
			ratio=step/sqrt(rxn->bindrad2);
			simLog(sim,2,"   mutual rms step length: %g\n",step);
			simLog(sim,2,"   step length / binding radius: %g (%s %s-limited)\n",ratio,ratio>0.1 && ratio<10?"somewhat":"very",ratio>1?"activation":"diffusion");
			simLog(sim,2,"   effective activation limited reaction rate: %g\n",actrxnrate(step,sqrt(rxn->bindrad2))/sim->dt); }
		*/

		if(rxn->rparamt!=RPbounce)
			// if(findreverserxn(sim,order,molec_num,r,&orderr,&rr)==1)
			//	simLog(sim,2," with reverse reaction %s, equilibrium constant is: %g\n",sim->rxnss[orderr]->rname[rr],rxn->rate/sim->rxnss[orderr]->rxn[rr]->rate);
		if(rxn->rparamt!=RPnone) simLog(sim,2,"   product placement method and parameter: %s %g\n",rxnrp2string(rxn->rparamt,string),rxn->rparam);
		for(prd=0;prd<rxn->nprod;prd++) {
			if(dotVVD(rxn->prdpos[prd],rxn->prdpos[prd],dim)>0) {
				// simLog(sim,2,"   product %s displacement:",sim->mols->spname[rxn->prdident[prd]]);
				simLog(sim,2,"	product%s displacement:",sim->mols->spname[rxn->prd[prd]->ident]);
				for(d=0;d<dim;d++) simLog(sim,2," %g",rxn->prdpos[prd][d]);
				simLog(sim,2,"\n"); }}

		// don't think the following part if neccessary
		/*
		// The following segment does not account for non-1 reaction probabilities.
		if(rxn->nprod==2 && sim->rxnss[2] && rxn->rparamt!=RPconfspread && rxn->rparamt!=RPbounce) {		// order=2, molec_num=2
			i=rxnpack_cplx(molec_num,rxn->ident,0);
			s=rxnpack_cplx(molec_num,rxn->prdsites,0);
			rr=sim->rxnss[2]->table[i][s];				// rxn order equals 1
			if(rr>=0){
				rxnr=sim->rxnss[2]->rxn[rr];
				if(rxnr->bindrad2>=0 && rxnr->rparamt!=RPconfspread) {
					dsum=MolCalcDifcSum(sim,rxn->ident[0],rxn->prdstate[0],rxn->ident[1],rxn->prdstate[1]);
					step=sqrt(2.0*sim->dt*dsum);
					pgem=1.0-numrxnrate(step,sqrt(rxnr->bindrad2),-1)/numrxnrate(step,sqrt(rxnr->bindrad2),rxn->unbindrad);
					rev=(rxnr->nprod==order);
					rev=rev && rxn->ident[0]==rxnr->ident[0] && rxn->ident[1]==rxnr->ident[1];		// Zn_sameset(rxnr->prdident,rxn->rctident,identlist,order);
					simLog(sim,2,"   probability of geminate %s reaction '%s' is %g\n",rev?"reverse":"continuation",rxnr->rname,pgem); }
			}}
		*/
		}

	simLog(sim,2,"\n");
	return; }


/* writereactions */
void writereactions(simptr sim,FILE *fptr) {
	int r,prd,d,rct,i,k,order;
	long int serno;
	rxnptr rxn;
	rxnssptr rxnss;
	char string[STRCHAR],string2[STRCHAR];
	enum MolecState ms,ms1,ms2;
	enum RevParam rparamt;

	fprintf(fptr,"# Reaction parameters\n");
	for(k=0;k<=MAXORDER;k++)
		if(sim->rxnss[k]) {
			rxnss=sim->rxnss[k];
			for(r=0;r<rxnss->totrxn;r++) {
				rxn=rxnss->rxn[r];
				if(!rxn) continue;
				order=rxn->order;
				if(rxn->cmpt) fprintf(fptr,"reaction_cmpt %s",rxn->cmpt->cname);
				else if(rxn->srf) fprintf(fptr,"reaction_surface %s",rxn->srf->sname);
				else fprintf(fptr,"reaction");
				fprintf(fptr," %s",rxn->rname);
				if(order==0) fprintf(fptr," 0");							// reactants
				for(rct=0;rct<order;rct++) {
					fprintf(fptr," %s",sim->mols->spname[rxn->rct[rct]->ident]);
					if(rxn->rct[rct]->rctstate!=MSsoln) fprintf(fptr,"(%s)",molms2string(rxn->rct[rct]->rctstate,string));
					if(rct<order-1) fprintf(fptr," +"); }
				fprintf(fptr," ->");
				if(rxn->nprod==0) fprintf(fptr," 0");					// products
				for(prd=0;prd<rxn->nprod;prd++) {
					// fprintf(fptr," %s",sim->mols->spname[rxn->prdident[prd]]);
					fprintf(fptr," %s",sim->mols->spname[rxn->prd[prd]->ident]);
					if(rxn->prd[prd]->prdstate!=MSsoln) fprintf(fptr,"(%s)",molms2string(rxn->prd[prd]->prdstate,string));
					if(prd<rxn->nprod-1) fprintf(fptr," +"); }
				if(rxn->rate>=0) fprintf(fptr," %g",rxn->rate);
				fprintf(fptr,"\n");

				if(order==1 && rxn->rct[0]->rctstate==MSsome) {
					for(ms=(MolecState)0;ms<MSMAX;ms=(MolecState)(ms+1))
						if(rxn->permit[ms])
							fprintf(fptr,"reaction_permit %s %s\n",rxn->rname,molms2string(ms,string)); }
				else if(order==2 && (rxn->rct[0]->rctstate==MSsome || rxn->rct[1]->rctstate==MSsome)) {
					for(ms1=(MolecState)0;ms1<MSMAX1;ms1=(MolecState)(ms1+1))
						for(ms2=(MolecState)0;ms2<MSMAX1;ms2=(MolecState)(ms2+1))
							if(rxn->permit[ms1*MSMAX1+ms2])
								fprintf(fptr,"reaction_permit %s %s %s\n",rxn->rname,molms2string(ms1,string),molms2string(ms2,string2)); }
				/*
				if(rxn->rparamt==RPconfspread)
					fprintf(fptr,"confspread_radius %s %g\n",rxn->rname,rxn->bindrad2<0?0:sqrt(rxn->bindrad2));
				*/
				if(rxn->rate<0) {
					if(order==0 && rxn->prob>=0) fprintf(fptr,"reaction_production %s %g\n",rxn->rname,rxn->prob);
					else if(order==1 && rxn->prob>=0) fprintf(fptr,"reaction_probability %s %g\n",rxn->rname,rxn->prob);
				//	else if(order==2 && rxn->bindrad2>=0) fprintf(fptr,"binding_radius %s %g\n",rxn->rname,sqrt(rxn->bindrad2));
				}
				if((order==2 && rxn->prob!=1 && rxn->rparamt!=RPconfspread) || (rxn->rparamt==RPconfspread && rxn->rate<0))
					fprintf(fptr,"reaction_probability %s %g\n",rxn->rname,rxn->prob);

				rparamt=rxn->rparamt;
				if(rparamt==RPirrev)
					fprintf(fptr,"product_placement %s irrev\n",rxn->rname);
				else if(rparamt==RPbounce || rparamt==RPpgem || rparamt==RPpgemmax || rparamt==RPratio || rparamt==RPunbindrad || rparamt==RPpgem2 || rparamt==RPpgemmax2 || rparamt==RPratio2)
					fprintf(fptr,"product_placement %s %s %g\n",rxn->rname,rxnrp2string(rparamt,string),rxn->rparam);
				else if(rparamt==RPoffset || rparamt==RPfixed) {
					for(prd=0;prd<rxn->nprod;prd++) {
						// fprintf(fptr,"product_placement %s %s %s\n",rxn->rname,rxnrp2string(rparamt,string),sim->mols->spname[rxn->prdident[prd]]);
						fprintf(fptr,"product_placement %s %s %s\n",rxn->rname,rxnrp2string(rparamt,string),sim->mols->spname[rxn->prd[prd]->ident]);
						for(d=0;d<sim->dim;d++) fprintf(fptr," %g",rxn->prdpos[prd][d]);
						fprintf(fptr,"\n"); }}

				if(rxn->prdserno) {
					fprintf(fptr,"reaction_serialnum %s",rxn->rname);
					for(prd=0;prd<rxn->nprod;prd++) {
						serno=rxn->prdserno[prd];
						if(serno==0) fprintf(fptr," new");
						else if(serno==-1) fprintf(fptr," r1");
						else if(serno==-2) fprintf(fptr," r2");
						else if(serno<=-10) fprintf(fptr," p%i",(int)(-9-serno));
						else fprintf(fptr," %li",serno); }
					fprintf(fptr,"\n"); }

				if(rxn->logserno) {											// reaction log
					fprintf(fptr,"reaction_log %s %s",rxn->logfile,rxn->rname);
					if(rxn->logserno->n==0)
						fprintf(fptr," all\n");
					else {
						for(i=0;i<rxn->logserno->n;i++)
							fprintf(fptr," %li",rxn->logserno->xs[i]);
						fprintf(fptr,"\n"); }}}}

	fprintf(fptr,"\n");
	return; }


/* checkrxnparams */
int checkrxnparams(simptr sim,int *warnptr) {
	int d,dim,warn,error,i1,i2,j,nspecies,r,i,ct,j1,j2,order,prd,prd2,molec_num;
	long int serno;
	molssptr mols;
	double minboxsize,vol,amax,vol2,vol3;
	rxnptr rxn,rxn1,rxn2;
	rxnssptr rxnss;
	char **spname,string[STRCHAR],string2[STRCHAR];
	enum MolecState ms1,ms2,ms;

	error=warn=0;
	dim=sim->dim;
	nspecies=sim->mols->nspecies;
	mols=sim->mols;
	spname=sim->mols->spname;

	for(order=0;order<=2;order++) {										// condition
		rxnss=sim->rxnss[order];
		if(rxnss) {
			if(rxnss->condition!=SCok) {
				warn++;
				simLog(sim,5," WARNING: order %i reaction structure %s\n",order,simsc2string(rxnss->condition,string)); }}}

	for(order=0;order<=2;order++) {										// maxspecies
		rxnss=sim->rxnss[order];
		if(rxnss) {
			if(!sim->mols) {error++;simLog(sim,10," SMOLDYN BUG: Reactions are declared, but not molecules\n");}
			// else if(sim->mols->maxspecies!=rxnss->maxspecies) {error++;simLog(sim,10," SMOLDYN BUG: number of molecule species differ between mols and rxnss\n");} 
		}}

	/*
	for(order=1;order<=2;order++) {									// reversible parameters
		rxnss=sim->rxnss[order];
		if(rxnss){
			for(r=0;r<rxnss->totrxn;r++) {
				rxn=rxnss->rxn[r];
				if(rxn->rparamt==RPpgemmaxw) {
					simLog(sim,5," WARNING: unspecified product parameter for reversible reaction %s.  Defaults are used.\n",rxn->rname);
					warn++; }}}}
	
	
	rxnss=sim->rxnss[2];															// check for multiple bimolecular reactions with same reactants
	if(rxnss) {
		for(i1=1;i1<nspecies;i1++)
			for(i2=1;i2<=i1;i2++)	{
				i=i1*rxnss->maxspecies+i2;
				for(j1=0;j1<rxnss->nrxn[i];j1++) {
					rxn1=rxnss->rxn[rxnss->table[i][j1]];
					for(j2=0;j2<j1;j2++) {
						rxn2=rxnss->rxn[rxnss->table[i][j2]];
						if(rxnallstates(rxn1) && rxnallstates(rxn2)) {
							simLog(sim,5," WARNING: multiply defined bimolecular reactions: %s(all) + %s(all)\n",spname[i1],spname[i2]);
							warn++; }
						else if(rxnallstates(rxn1)) {
							for(ms2=0;ms2<=MSMAX1;ms2=ms2+1) {
								ms=MSsoln*MSMAX1+ms2;
								if(rxn2->permit[ms]) {
									simLog(sim,5," WARNING: multiply defined bimolecular reactions: %s(all) + %s(%s)\n",spname[i1],spname[i2],molms2string(ms2,string2));
									warn++; }}}
						else if(rxnallstates(rxn2)) {
							for(ms1=0;ms1<=MSMAX1;ms1=ms1+1) {
								ms=ms1*MSMAX1+MSsoln;
								if(rxn1->permit[ms]) {
									simLog(sim,5," WARNING: multiply defined bimolecular reactions: %s(%s) + %s(all)\n",spname[i1],molms2string(ms1,string),spname[i2]);
									warn++; }}}
						else {
							for(ms1=0;ms1<MSMAX1;ms1=ms1+1)
								for(ms2=0;ms2<=ms1;ms2=ms2+1) {
									ms=ms1*MSMAX1+ms2;
									if(rxn1->permit[ms] && rxn2->permit[ms]) {
										simLog(sim,5," WARNING: multiply defined bimolecular reactions: %s(%s) + %s(%s)\n",spname[i1],molms2string(ms1,string),spname[i2],molms2string(ms2,string2));
										warn++; }}}}}}}

	rxnss=sim->rxnss[2];
	if(rxnss){
		for(r=0;r<rxnss->totrxn;r++){
			rxn=rxnss->rxn[r];
			i1=rxn->ident[0];
			i2=rxn->ident[1];
			if(mols->difm[i1][MSsoln] && rxn->rate){							// warn that difm ignored for reaction rates
				simLog(sim,5," WARNING: diffusion matrix for %s was ignored for calculating rate for reaction %s\n", spname[i1],rxn->rname);
				warn++;
			}
			if(mols->difm[i2][MSsoln] && rxn->rate){
				simLog(sim,5," WARNING: diffusion matrix for %s was ignored for calculating rate for reaction %s\n", spname[i2],rxn->rname);
				warn++;
			}
			
			if(mols->drift[i1][MSsoln] && rxn->rate){							// warn that drift ignored for reaction rate
				simLog(sim,5," WARNING: drift vector for %s was ignored for calculating rate for reaction %s\n", spname[i1],rxn->rname);
				warn++;
			}
			if(mols->drift[i2][MSsoln] && rxn->rate){
				simLog(sim,5," WARNING: drift vector for %s was ignored for calculating rate for reaction %s\n", spname[i2],rxn->rname);
	
			}
		}
	}

	for(order=1;order<=2;order++){									
		rxnss=sim->rxnss[order];
		if(rxnss){
			for(r=0;r<rxnss->totrxn;r++){
				rxn=rxnss->rxn[r];
				molec_num=(order>rxn->nprod)?order:rxn->nprod;
				if(rxn->permit[order==1?MSsoln:MSsoln*MSMAX1+MSsoln]){			// product surface-bound states imply reactant surface-bound			
					for(prd=0;prd<molec_num;prd++){
						if(rxn->prdstate[prd]!=MSsoln)	simLog(sim,10," ERROR: a product of reaction %s is surface-bound, but no reactant is \n", rxn->rname);	error++;
				}}
				if(rxn->cmpt){													// reaction compartment
					if(order==0 && rxn->cmpt->volume<=0){
						simLog(sim,10," ERROR: a product of reaction %s cannot work in compartment %s because the compartment has no volume\n", rxn->rname, rxn->cmpt->cname);
						error++;	
				}}
				if(rxn->srf){
					if(order==0 && surfacearea(rxn->srf,sim->dim,NULL)<=0) {	// reaction surface
						simLog(sim,10," ERROR: reaction %s cannot work on surface %s because the surface has no area\n",rxn->rname,rxn->srf->sname);
						error++;
				}}

				if(rxn->prdserno) {												// reaction serial numbers			
					j=0;
					for(prd=0;prd<rxn->nprod;prd++) {
						serno=rxn->prdserno[prd];
						if(serno<=-10) j=1;
						else if(serno>0) j=1;
						else if(serno==-1 || serno==-2)
							for(prd2=prd+1;prd2<rxn->nprod;prd2++)
								if(rxn->prdserno[prd2]==serno) j=1; }
					if(j) {
						simLog(sim,5," WARNING: multiple molecules might have the same serial number due to reaction %s\n",rxn->rname);
						warn++;
				}}
	}}}							


	rxnss=sim->rxnss[0];														// order 0 reactions
	if(rxnss)
		for(r=0;r<rxnss->totrxn;r++) {
			rxn=rxnss->rxn[r];
			if(!rxn->srf) {
				for(prd=0;prd<rxn->nprod;prd++)
					if(rxn->prdstate[prd]!=MSsoln) {
						simLog(sim,10," ERROR: order 0 reaction %s has surface-bound products but no surface listed\n",rxn->rname);
						error++; }}
			if(rxn->prob<0) {
				simLog(sim,5," WARNING: reaction rate not set for reaction order 0, name %s\n",rxn->rname);
				rxn->prob=0;
				warn++; }}

	rxnss=sim->rxnss[1];														// order 1 reactions
	if(rxnss)
		for(r=0;r<rxnss->totrxn;r++) {
			rxn=rxnss->rxn[r];
			if(rxn->prob<0) {
				simLog(sim,5," WARNING: reaction rate not set for reaction order 1, name %s\n",rxn->rname);
				rxn->prob=0;
				warn++; }
			else if(rxn->prob>0 && rxn->prob<10.0/(double)RAND2_MAX) {
				simLog(sim,5," WARNING: order 1 reaction %s probability is at lower end of random number generator resolution\n",rxn->rname);
				warn++; }
			else if(rxn->prob>((double)RAND2_MAX-10.0)/(double)RAND2_MAX && rxn->prob<1.0) {
				simLog(sim,5," WARNING: order 1 reaction %s probability is at upper end of random number generator resolution\n",rxn->rname);
				warn++; }
			else if(rxn->prob>0.2) {
				simLog(sim,5," WARNING: order 1 reaction %s probability is quite high\n",rxn->rname);
				warn++; }
			if(rxn->tau<5*sim->dt) {
				simLog(sim,5," WARNING: order 1 reaction %s time constant is only %g times longer than the simulation time step\n",rxn->rname,rxn->tau/sim->dt);
				warn++; }}

	minboxsize=sim->boxs->size[0];
	for(d=1;d<dim;d++)
		if(sim->boxs->size[d]<minboxsize) minboxsize=sim->boxs->size[d];

	rxnss=sim->rxnss[2];															// order 2 reactions
	if(rxnss) {
		for(r=0;r<rxnss->totrxn;r++) {
			rxn=rxnss->rxn[r];
			if(rxn->bindrad2<0) {
				if(rxn->rparamt==RPconfspread) simLog(sim,5," WARNING: confspread radius not set for order 2 reaction %s\n",rxn->rname);
				else simLog(sim,5," WARNING: reaction rate not set for reaction order 2, name %s\n",rxn->rname);
				rxn->bindrad2=0;
				warn++; }
			else if(sqrt(rxn->bindrad2)>minboxsize) {
				if(rxn->rparamt==RPconfspread) simLog(sim,5," WARNING: confspread radius for order 2 reaction %s is larger than box dimensions\n",rxn->rname);
				else if(rxn->rparamt==RPbounce) simLog(sim,5," WARNING: bounce radius for order 2 reaction %s is larger than box dimensions\n",rxn->rname);
				else simLog(sim,5," WARNING: binding radius for order 2 reaction %s is larger than box dimensions\n",rxn->rname);
				warn++; }
			if(rxn->prob<0 || rxn->prob>1) {
				simLog(sim,10," ERROR: reaction %s probability is not between 0 and 1\n",rxn->rname);
				error++; }
			else if(rxn->prob<1 && rxn->rparamt!=RPconfspread && rxn->rparamt!=RPbounce) {
				simLog(sim,5," WARNING: reaction %s probability is not accounted for in rate calculation\n",rxn->rname);
				warn++; }
			if(rxn->tau<5*sim->dt) {
				simLog(sim,5," WARNING: order 2 reaction %s time constant is only %g times longer than the simulation time step\n",rxn->rname,rxn->tau/sim->dt);
				warn++; }}}
	
	rxnss=sim->rxnss[2];															// more order 2 reactions
	if(rxnss) {
		vol=systemvolume(sim);
		vol2=0;
		for(i=1;i<nspecies;i++) {
			amax=0;
			for(i1=1;i1<nspecies;i1++)
				for(j=0;j<rxnss->nrxn[i*rxnss->maxspecies+i1];j++) {
					r=rxnss->table[i*rxnss->maxspecies+i1][j];
					rxn=rxnss->rxn[r];
					if(amax<sqrt(rxn->bindrad2)) amax=sqrt(rxn->bindrad2); }
			ct=molcount(sim,i,NULL,MSsoln,NULL,-1);
			vol3=ct*4.0/3.0*PI*amax*amax*amax;
			vol2+=vol3;
			if(vol3>vol/10.0) {
				simLog(sim,5," WARNING: reactive volume of %s is %g %% of total volume\n",spname[i],vol3/vol*100);
				warn++; }}
		if(vol2>vol/10.0) {
			simLog(sim,5," WARNING: total reactive volume is a large fraction of total volume\n");
			warn++; }}
	*/

	if(warnptr) *warnptr=warn;
	return error; }


/******************************************************************************/
/*************************** parameter calculations ***************************/
/******************************************************************************/


/* rxnsetrate */
int rxnsetrate(simptr sim,int molec_num,int r_indx,char *erstr) {
	rxnssptr rxnss;
	int i,j,k,s,i1,i2,rev,o2,r2,permit,order;
	int i_p,s_p;
	rxnptr rxn,rxn2;
	double vol,rate3,dsum,rparam,unbindrad,prob,product,sum;
	enum MolecState ms,ms1,ms2,statelist[MAXORDER];
	enum RevParam rparamt;

	rxnss=sim->rxnss[molec_num];
	rxn=rxnss->rxn[r_indx];
	// if(!rxn) return 0;
	order=rxn->order;


	/*
	sum=0;
	if(order==1){
		for(;rxn;rxn=rxn->next_rxn) sum+=rxn->rate;
		rxn=rxnss->rxn[r];
		product=1.0;
	}
	*/
	// for(;rxn;rxn=rxn->next_rxn){				// didn't really consider order 1, molec_num=2 reactions starting with the same reactants
		if(rxn->disable) rxn->prob=0; 
		else if(rxn->rparamt==RPconfspread) {										// confspread
			if(rxn->rate<0) {sprintf(erstr,"reaction %s rate is undefined",rxn->rname);return 1;}
			if(rxn->nprod!=order) {sprintf(erstr,"confspread reaction %s has a different number of reactants and products",rxn->rname);return 3;}
			if(rxn->rate>=0) rxn->prob=1-exp(-sim->dt*rxn->rate);			// 1.0-exp(-sim->dt*rxn->rate); 

		}

		if(order==0) {															// order 0
			if(rxn->rate<0) {sprintf(erstr,"reaction %s rate is undefined",rxn->rname);return 1;}
			if(rxn->cmpt) vol=rxn->cmpt->volume;
			else if(rxn->srf) vol=surfacearea(rxn->srf,sim->dim,NULL);
			else vol=systemvolume(sim);
			rxn->prob=rxn->rate*sim->dt*vol; }

		if(order==1) {														
			// for(ms=0;ms<MSMAX && !rxn->permit[ms];ms=ms+1);		// determine first state that this is permitted for (usually only state)
			if(rxn->rate<0) {sprintf(erstr,"reaction %s rate is undefined",rxn->rname);return 1;}
			else if(rxn->rate==0) rxn->prob=0;
			else if(ms==MSMAX) rxn->prob=0;
			else {
				rxn->prob=1.0-exp(-rxn->rate*sim->dt);

				// rxn->prob=rxn->rate/sum*(1.0-exp(-sum*sim->dt));	
				/*	
				for(rxn2=rxnss->rxn[r];rxn2;rxn2=rxn2->next_rxn){
					if(rxn2==rxn) break;
					if(rxn2->permit[ms]) {
						prob=rxn2->prob*product;					
						product*=1.0-prob;
					}
				}
				rxn->prob/=product;	
				printf("%s, order=%d, prob=%g\n", rxn->rname, order, rxn->prob);
				if(!(rxn->prob>=0 && rxn->prob<=1)) {sprintf(erstr,"reaction %s probability is %g, out of range",rxn->rname,rxn->prob); return 5;}			
				*/			

				/*	
				rev=findreverserxn(sim,order,r,&o2,&r2);			// set own reversible parameter if needed
				if(rev>0 && o2==2) {
					if(rxn->rparamt==RPnone) {
						rxn->rparamt=RPpgemmaxw;
						rxn->rparam=0.2; }}
				*/
			}
		}
		
		else if(order==2) {													
			if(rxn->rate<0) {
				if(rxn->prob<0) rxn->prob=1;
				sprintf(erstr,"reaction rate %s is undefined",rxn->rname);
				return 1; }
			permit=rxnreactantstate(rxn,statelist,1);
			if(rxn->prob<0) rxn->prob=1;
			/*
			i1=rxn->rct[0]->ident;
			i2=rxn->rct[1]->ident;
			ms1=statelist[0];
			ms2=statelist[1];

			rate3=rxn->rate;
			if(i1==i2) rate3*=2;				// same reactants
			if((ms1==MSsoln && ms2!=MSsoln) || (ms1!=MSsoln && ms2==MSsoln)) rate3*=2;	// one surface, one solution
			dsum=MolCalcDifcSum(sim,i1,ms1,i2,ms2);

			rev=findreverserxn(sim,order,r,&o2,&r2);
			if(rev>0 && o2==2) {						// set own reversible parameter if needed
				if(rxn->rparamt==RPnone) {
					rxn->rparamt=RPpgemmaxw;
					rxn->rparam=0.2; }}

			if(rev==1) {								// set incoming reversible parameter if needed, and then use it
				if(sim->rxnss[o2]->rxn[r2]->rparamt==RPnone) {
					sim->rxnss[o2]->rxn[r2]->rparamt=RPpgemmaxw;
					sim->rxnss[o2]->rxn[r2]->rparam=0.2; }
				rparamt=sim->rxnss[o2]->rxn[r2]->rparamt;
				rparam=sim->rxnss[o2]->rxn[r2]->rparam; }
			else {
				rparamt=RPnone;
				rparam=0; }

			if(rxn->prob<0) rxn->prob=1;
			if(!permit) rxn->bindrad2=0;
			else if(rate3<=0) rxn->bindrad2=0;
			else if(dsum<=0) {sprintf(erstr,"Both diffusion coefficients are 0");return 4;}
			else if(rparamt==RPunbindrad) rxn->bindrad2=bindingradius(rate3,sim->dt,dsum,rparam,0);
			else if(rparamt==RPbounce) rxn->bindrad2=bindingradius(rate3,sim->dt,dsum,rparam,0);
			else if(rparamt==RPratio) rxn->bindrad2=bindingradius(rate3,sim->dt,dsum,rparam,1);
			else if(rparamt==RPpgem) rxn->bindrad2=bindingradius(rate3*(1.0-rparam),sim->dt,dsum,-1,0);
			else if(rparamt==RPpgemmax || rparamt==RPpgemmaxw) {
				rxn->bindrad2=bindingradius(rate3,sim->dt,dsum,0,0);
				unbindrad=unbindingradius(rparam,sim->dt,dsum,rxn->bindrad2);
			if(unbindrad>0) rxn->bindrad2=bindingradius(rate3*(1.0-rparam),sim->dt,dsum,-1,0); }
			else rxn->bindrad2=bindingradius(rate3,sim->dt,dsum,-1,0);

			rxn->bindrad2*=rxn->bindrad2; 
			*/
		}
	// }
	return 0; }


/* rxnsetrates */
int rxnsetrates(simptr sim,int molec_num,char *erstr) {
	rxnssptr rxnss;
	int r_indx,er;

	rxnss=sim->rxnss[molec_num];
	if(!rxnss) return -1;

	for(r_indx=0;r_indx<rxnss->totrxn;r_indx++) {
		er=rxnsetrate(sim,rxnss->molec_num,r_indx,erstr);
		if(er>1) return r_indx; }		// error code of 1 is just a warning, not an error

	return -1; }

/* rxnsetproduct */
/*
int rxnsetproduct(simptr sim,int molec_num,int r,char *erstr) {
	rxnssptr rxnss;
	rxnptr rxn,rxnr;
	int er,dim,nprod,orderr,rr,rev,d;
	double rpar,dc1,dc2,dsum,bindradr;
	enum RevParam rparamt;
	enum MolecState ms1,ms2;

	rxnss=sim->rxnss[molec_num];
	rxn=rxnss->rxn[r];
	if(!rxn) return 0;
	nprod=rxn->nprod;
	rpar=rxn->rparam;
	rparamt=rxn->rparamt;
	er=0;
	dim=sim->dim;

	if(nprod==0) {
		if(!(rparamt==RPnone || rparamt==RPirrev)) {
			sprintf(erstr,"Illegal product parameter because reaction has no products");er=1; }}		

	else if(nprod==1) {
		if(!(rparamt==RPnone || rparamt==RPirrev || rparamt==RPbounce || rparamt==RPconfspread || rparamt==RPoffset || rparamt==RPfixed)) {
			sprintf(erstr,"Illegal product parameter because reaction only has one product");er=2; }
		else if(rparamt==RPoffset || rparamt==RPfixed);
		else {
			rxn->unbindrad=0;
			for(d=0;d<dim;d++) rxn->prdpos[0][d]=0; }}

	else if(nprod==2) {
		ms1=rxn->prd[0]->prdstate;
		ms2=rxn->prd[1]->prdstate;
		if(ms1==MSbsoln) ms1=MSsoln;
		if(ms2==MSbsoln) ms2=MSsoln;
		dc1=MolCalcDifcSum(sim,rxn->prd[0]->ident,ms1,0,MSnone);
		dc2=MolCalcDifcSum(sim,rxn->prd[1]->ident,ms2,0,MSnone);
		dsum=dc1+dc2;
		// rev=findreverserxn(sim,order,r,&orderr,&rr);
		rev=0;
		if(rev==0) {
			if(rparamt==RPpgem || rparamt==RPpgemmax || rparamt==RPpgemmaxw || rparamt==RPratio || rparamt==RPpgem2 || rparamt==RPpgemmax2 || rparamt==RPratio2) {
				sprintf(erstr,"Illegal product parameter because products don't react");er=3; }
			else if(rparamt==RPunbindrad) {
				if(dsum==0) dsum=(dc1=1.0)+(dc2=1.0);
				rxn->unbindrad=rpar;
				rxn->prdpos[0][0]=rpar*dc1/dsum;
				rxn->prdpos[1][0]=-rpar*dc2/dsum; }
			else if(rparamt==RPbounce) {
				if(dsum==0) dsum=(dc1=1.0)+(dc2=1.0);
				if(rpar>=0) {
					rxn->unbindrad=rpar;
					rxn->prdpos[0][0]=rpar*dc1/dsum;
					rxn->prdpos[1][0]=-rpar*dc2/dsum; }
				else {
					rxn->unbindrad=-1;
					rxn->prdpos[0][0]=dc1/dsum;
					rxn->prdpos[1][0]=-dc2/dsum; }}}

		else {
			rxnr=sim->rxnss[orderr]->rxn[rr];
			if(rxnr->bindrad2>=0) bindradr=sqrt(rxnr->bindrad2);
			else bindradr=-1;

			if(rparamt==RPnone) {
				sprintf(erstr,"Undefined product placement for reversible reaction");er=5; }
			else if(rparamt==RPoffset || rparamt==RPfixed || rparamt==RPconfspread);
			else if(rparamt==RPirrev) {
				rxn->unbindrad=0;
				for(d=0;d<dim;d++) rxn->prdpos[0][d]=rxn->prdpos[1][d]=0; }
			else if(dsum<=0) {						// all below options require dsum > 0
				sprintf(erstr,"Cannot set unbinding distance because sum of product diffusion constants is 0");er=4; }		
			else if(rparamt==RPunbindrad) {
				rxn->unbindrad=rpar;
				rxn->prdpos[0][0]=rpar*dc1/dsum;
				rxn->prdpos[1][0]=-rpar*dc2/dsum; }
			else if(rparamt==RPbounce) {
				if(rpar>=0) {
					rxn->unbindrad=rpar;
					rxn->prdpos[0][0]=rpar*dc1/dsum;
					rxn->prdpos[1][0]=-rpar*dc2/dsum; }
				else {
					rxn->unbindrad=-1;
					rxn->prdpos[0][0]=dc1/dsum;
					rxn->prdpos[1][0]=-dc2/dsum; }}
			else if(rxnr->bindrad2<0) {			// all below options require bindrad2 >= 0
				sprintf(erstr,"Binding radius of reaction products is undefined");er=6; }
			else if(rparamt==RPratio || rparamt==RPratio2) {
				rxn->unbindrad=rpar*bindradr;
				rxn->prdpos[0][0]=rpar*bindradr*dc1/dsum;
				rxn->prdpos[1][0]=-rpar*bindradr*dc2/dsum; }
      		else if(rxnr->bindrad2==0) {    // set to 0 if bindrad2 == 0
        		rxn->unbindrad=0;
        		rxn->prdpos[0][0]=0;
        		rxn->prdpos[1][0]=0; }
			else if(rparamt==RPpgem || rparamt==RPpgem2) {
				rpar=unbindingradius(rpar,sim->dt,dsum,bindradr);
				if(rpar==-2) {
					sprintf(erstr,"Cannot create an unbinding radius due to illegal input values");er=7; }		
				else if(rpar<0) {
					sprintf(erstr,"Maximum possible geminate binding probability is %g",-rpar);er=8; }		
				else {
					rxn->unbindrad=rpar;
					rxn->prdpos[0][0]=rpar*dc1/dsum;
					rxn->prdpos[1][0]=-rpar*dc2/dsum; }}
			else if(rparamt==RPpgemmax || rparamt==RPpgemmaxw || rparamt==RPpgemmax2) {
				rpar=unbindingradius(rpar,sim->dt,dsum,bindradr);
				if(rpar==-2) {
					sprintf(erstr,"Illegal input values");er=9; }		
				else if(rpar<=0) {
					rxn->unbindrad=0; }
				else if(rpar>0) {
					rxn->unbindrad=rpar;
					rxn->prdpos[0][0]=rpar*dc1/dsum;
					rxn->prdpos[1][0]=-rpar*dc2/dsum; }}
			else {
				simLog(sim,10,"BUG in rxnsetproduct");er=10; }}}		

	return er; }
*/


/* rxnsetproducts */
int rxnsetproducts(simptr sim,int molec_num,char *erstr) {
	rxnssptr rxnss;
	int r,er;

	rxnss=sim->rxnss[molec_num];
	if(!rxnss) return -1;
	for(r=0;r<rxnss->totrxn;r++) {
		// er=rxnsetproduct(sim,molec_num,r,erstr);
		if(er) return r; }
	return -1; }


/* rxncalcrate */
double rxncalcrate(simptr sim,int molec_num,int r,double *pgemptr) {
	rxnssptr rxnss;
	double ans,vol,rpar;
	int k,i1,i2,i,j,s,r2,rev,o2,permit;
	double step,a,bval,product,probthisrxn,prob,sum,ratesum;
	rxnptr rxn,rxnr,rxn2;
	enum MolecState ms1,ms2,statelist[MAXORDER];
	enum RevParam rparamt;
	// int i_p,s_p;

	if(!sim) return -1;
	rxnss=sim->rxnss[molec_num];
	rxn=rxnss->rxn[r];
	if(!rxn) return -1;

	// convert all g_slist in the hash_table into circular list, so not doing the probability adjustment
	/*
	sum=0;
	if(order==1){
		for(;rxn;rxn=rxn->next_rxn) sum+=rxn->rate;
		rxn=rxnss->rxn[r];	
	}

	for(;rxn;rxn=rxn->next_rxn){
		if(order==0) {																// order 0
			if(rxn->cmpt) vol=rxn->cmpt->volume;
			else if(rxn->srf) vol=surfacearea(rxn->srf,sim->dim,NULL);
			else vol=systemvolume(sim);
			if(rxn->prob<0) ans=0;
			else ans=rxn->prob/sim->dt/vol;
		 }

		else if(order==1) {															// order 1
			ans=0;
			for(ms1=0;ms1<MSMAX && !rxn->permit[ms1];ms1=ms1+1);
			if(rxn->prob>0 && ms1!=MSMAX) {
				ratesum=-log(1.0-sum/sim->dt);
				ans=ratesum*probthisrxn/sum;
				printf("%s, k=%d, sum=%f, ratesum=%g, probthisrxn=%g, ans=%g\n", rxn->rname, k, sum, ratesum, probthisrxn, ans);
			}
		}
	}	
	*/
	
	/*
	if(molec_num==2) {															// order 2
		if(rxn->bindrad2<0 || rxn->prob<0) ans=0;
		else {
			i1=rxn->rct[0]->ident;
			i2=rxn->rct[1]->ident;

			// for(k=0;k<rxnss->nrxn[j];k++){
			//	if(r==rxnss->table[j][k]){
			permit=rxnreactantstate(rxn,statelist,1);
			ms1=statelist[0];
			ms2=statelist[1];
			if(!permit) return 0;
			if(rxn->rparamt==RPconfspread) return -log(1.0-rxn->prob)/sim->dt;
			step=sqrt(2.0*MolCalcDifcSum(sim,i1,ms1,i2,ms2)*sim->dt);
			a=sqrt(rxn->bindrad2);
			// rev=findreverserxn(sim,order,r,&o2,&r2);
			rev=0;
			if(rev==1) {
				rparamt=sim->rxnss[o2]->rxn[r2]->rparamt;
				rpar=sim->rxnss[o2]->rxn[r2]->rparam; }
			else {
				rparamt=RPnone;
				rpar=0; }
			if(rparamt==RPpgem || (rparamt==RPbounce && rpar>=0) || rparamt==RPpgemmax || rparamt==RPpgemmaxw || rparamt==RPratio || rparamt==RPoffset || rparamt==RPfixed) {
				rxnr=sim->rxnss[o2]->rxn[r2];
				bval=distanceVVD(rxnr->prdpos[0],rxnr->prdpos[1],sim->dim);
				ans=numrxnrate(step,a,bval); }
			else
				ans=numrxnrate(step,a,-1);
				ans/=sim->dt;
				if(i1==i2) ans/=2.0;
				if(!rxn->permit[MSsoln*MSMAX1+MSsoln]) ans/=2.0;
				//	}}
		}}
		else ans=0;
		*/
	
		/*
		if(pgemptr) {
			if(rxn->nprod!=2) *pgemptr=-1; // || findreverserxn(sim,order,r,&o2,&r2)==0) *pgemptr=-1;
			else {
				step=sqrt(2.0*MolCalcDifcSum(sim,rxn->ident[0],rxn->prdstate[0],rxn->ident[1],rxn->prdstate[1])*sim->dt);
				bval=distanceVVD(rxn->prdpos[0],rxn->prdpos[1],sim->dim);
				rxnr=sim->rxnss[o2]->rxn[r2];
				a=sqrt(rxnr->bindrad2);
				*pgemptr=1.0-numrxnrate(step,a,-1)/numrxnrate(step,a,bval); }}
		*/

	return ans; }

/* rxncalctau */
void rxncalctau(simptr sim,int molec_num) {
	rxnssptr rxnss;
	rxnptr rxn;
	int r,k;
	GSList *rxn_sl;
	double rate,vol,conc1,conc2;

	rxnss=sim->rxnss[molec_num];
	if(!rxnss) return;

	if(molec_num==1) {
		for(r=0;r<rxnss->totrxn;r++) {
			rxn=rxnss->rxn[r];
			rate=rxncalcrate(sim,1,r,NULL);
			rxn->tau=1.0/rate; 
	}}

	else if(molec_num==2) {
		vol=systemvolume(sim);
		for(r=0;r<rxnss->totrxn;r++) {
		rxn=rxnss->rxn[r];
		// if(!rxn) continue;
		conc1=(double)molcount(sim,rxn->rct[0]->ident,NULL,MSall,NULL,-1)/vol;
		conc2=(double)molcount(sim,rxn->rct[1]->ident,NULL,MSall,NULL,-1)/vol;
		rate=rxncalcrate(sim,2,r,NULL);
		if(rxn->rparamt==RPconfspread) rxn->tau=1.0/rate;
		else rxn->tau=(conc1+conc2)/(rate*conc1*conc2); 
	}}

	return; }


/******************************************************************************/
/****************************** structure set up ******************************/
/******************************************************************************/


/* rxnsetcondition */
void rxnsetcondition(simptr sim,int order,enum StructCond cond,int upgrade) {
	int o1,o2;

	if(!sim) return;
	if(order<0) {
		o1=0;
		o2=2; }
	else if(order<=2)
		o1=o2=order;
	else
		return;

	for(order=o1;order<=o2;order++) {
		if(sim->rxnss[order]) {
			if(upgrade==0 && sim->rxnss[order]->condition>cond) sim->rxnss[order]->condition=cond;
			else if(upgrade==1 && sim->rxnss[order]->condition<cond) sim->rxnss[order]->condition=cond;
			else if(upgrade==2) sim->rxnss[order]->condition=cond;
			if(sim->rxnss[order]->condition<sim->condition) {
				cond=sim->rxnss[order]->condition;
				simsetcondition(sim,cond==SCinit?SClists:cond,0); }}}
	return; }


/* RxnSetValue */
int RxnSetValue(simptr sim,const char *option,rxnptr rxn,double value) {
	int er;

	er=0;
	if(!rxn || !option) er=1;

	else if(!strcmp(option,"rate")) {
		if(rxn->rate!=-1) er=3;
		if(value<0) er=4;
		rxn->rate=value; }

	else if(!strcmp(option,"confspreadrad")) {
		if(rxn->rparamt==RPconfspread) er=3;
		rxn->rparamt=RPconfspread;
		if(value<0) er=4;
		rxn->bindrad=value; }

	else if(!strcmp(option,"bindrad")) {
		if(rxn->rparamt==RPconfspread) er=3;
		if(value<0) er=4;
		rxn->bindrad=value; }


	else if(!strcmp(option,"prob")) {
		if(value<0) er=4;
		// if(rxn->rxnss->order>0 && value>1) er=4;
		rxn->prob=value; }

	else if(!strcmp(option,"disable")) {
		rxn->disable=(int) value; }

	else er=2;
	rxnsetcondition(sim,-1,SCparams,0);
	return er; }


/* RxnSetRevparam */
int RxnSetRevparam(simptr sim,rxnptr rxn,enum RevParam rparamt,double rparam,int prd,double *pos,int dim) {
	int d,er;

	er=0;
	if(rxn->rparamt!=RPnone) er=1;
	rxn->rparamt=rparamt;

	if(rparamt==RPnone || rparamt==RPirrev || rparamt==RPconfspread);
	else if(rparamt==RPpgem || rparamt==RPpgemmax || rparamt==RPpgemmaxw || rparamt==RPpgem2 || rparamt==RPpgemmax2) {
		if(!(rparam>0 && rparam<=1)) er=2;
		rxn->rparam=rparam; }
	else if(rparamt==RPbounce)
		rxn->rparam=rparam;
	else if(rparamt==RPratio || rparamt==RPunbindrad || rparamt==RPratio2) {
		if(rparam<0) er=2;
		rxn->rparam=rparam; }
	else if(rparamt==RPoffset || rparamt==RPfixed) {
		er=0;
		if(prd<0 || prd>=rxn->nprod) er=4;
		else if(!pos) er=5;
		else for(d=0;d<dim;d++) rxn->prdpos[prd][d]=pos[d]; }
	else
		er=3;
	rxnsetcondition(sim,-1,SCparams,0);
	return er; }


/* RxnSetPermit */
void RxnSetPermit(simptr sim,rxnptr rxn,int value){
	enum MolecState ms; 
	// enum nms2o,mslist[MSMAX1];
	// int set;
	int n;
	// static int recurse;
	
	if(rxn->molec_num==0) return;
	for(n=0;n<rxn->molec_num;n++){
		ms=rxn->rct[n]->rctstate;
		rxn->permit[ms+n*MSMAX1]=value;
	}

	/*
	if(order==0) return;
	nms2o=(MolecState)intpower(MSMAX1,order);
	for(ms=(MolecState)0;ms<nms2o;ms=(MolecState)(ms+1)) {
		rxnunpackstate(order,ms,mslist);
		set=1;
		for(ord=0;ord<order && set;ord++)
			if(!(rctstate[ord]==MSall || rctstate[ord]==mslist[ord])) set=0;		// don't understand what this is for
		if(set) rxn->permit[ms]=value; }

	if(order==2 && rxn->rct[0]->ident==rxn->rct[1]->ident && recurse==0) {
		recurse=1;
		mslist[0]=rctstate[1];
		mslist[1]=rctstate[0];
		RxnSetPermit(sim,rxn,value);
		recurse=0;
	}
	*/
	rxnsetcondition(sim,-1,SCparams,0);
	surfsetcondition(sim->srfss,SClists,0);
	return; }


/* RxnSetCmpt */
void RxnSetCmpt(rxnptr rxn,compartptr cmpt) {
	rxn->cmpt=cmpt;	
	return; }


/* RxnSetSurface */
void RxnSetSurface(rxnptr rxn,surfaceptr srf) {
	rxn->srf=srf;
	return; }


/* RxnSetPrdSerno */
int RxnSetPrdSerno(rxnptr rxn,long int *prdserno) {
	int prd;

	if(!rxn->prdserno) {
		rxn->prdserno=(long int*) calloc(rxn->nprod,sizeof(long int));
		if(!rxn->prdserno) return 1;
		for(prd=0;prd<rxn->nprod;prd++)
			rxn->prdserno[prd]=0; }

	for(prd=0;prd<rxn->nprod;prd++)
		rxn->prdserno[prd]=prdserno[prd];

	return 0; }


/* RxnSetLog */
int RxnSetLog(simptr sim,char *filename,rxnptr rxn,listptrli list,int turnon) {
	int order,r;
	rxnssptr rxnss;

	if(!rxn) {												// all reactions
		for(order=0;order<=2;order++) {
			rxnss=sim->rxnss[order];
			if(rxnss)
				for(r=0;r<rxnss->totrxn;r++) {
					rxn=rxnss->rxn[r];
					RxnSetLog(sim,filename,rxn,list,turnon); }}}

	if(turnon) {												// turn on logging
		if(!list) {
			CHECKMEM(rxn->logserno=List_AllocLI(0)); }
		else {
			if(!rxn->logserno) rxn->logserno=list;
			else {
				CHECKMEM(List_AppendToLI(rxn->logserno,list)==0);
				List_FreeLI(list); }}

		if(!rxn->logfile) {
			CHECKMEM(rxn->logfile=EmptyString()); }
		strcpy(rxn->logfile,filename); }

	else {															// turn off logging
		if(!list) {
			List_FreeLI(rxn->logserno);
			rxn->logserno=NULL; }
		else {
			List_RemoveFromLI(rxn->logserno,list);
			if(rxn->logserno->n==0) {
				List_FreeLI(rxn->logserno);
				rxn->logserno=NULL; }
			List_FreeLI(list); }}

	return 0;

 failure:
	return 1; }


void rxnsetup(simptr sim, rxnssptr rxnss, rxnptr rxn, char *rname, int order, int molec_num, int nprod, rct_mptr rct1, rct_mptr rct2, prd_mptr prd1, prd_mptr prd2, compartptr cmpt, surfaceptr srf, double flt1){
	int j,k,d, rct_num,prd_num;
	
	rct_num=rct2?2:1;
	prd_num=prd2?2:1;

	rxn->rxnss=rxnss;
	rxn->rname=EmptyString();
	strncpy(rxn->rname, rname,STRCHAR-1);
	rxn->rname[STRCHAR-1]='\0';
	rxn->order=order;
	rxn->molec_num=molec_num;
 	rxn->nprod=nprod;
	
	rxn->rct=(rct_mptr*) calloc(rct_num,sizeof(rct_mptr));
	rxn->prd=(prd_mptr*) calloc(prd_num,sizeof(prd_mptr));
	for(k=0;k<molec_num;k++){								// notice for reactants, this is molec_num, not order
		if(k==0) rxn->rct[k]=rct1;
		if(k==1) rxn->rct[k]=rct2;
	}
	for(k=0;k<prd_num;k++){									// for products, this is nprod, not molec_num
		if(k==0) rxn->prd[k]=prd1;
		if(k==1) rxn->prd[k]=prd2;
	}

	RxnSetPermit(sim,rxn,1);
	rxn->prdpos=(double**)calloc(prd_num,sizeof(double*));
	for(k=0;k<prd_num;k++){
		rxn->prdpos[k]=(double*)calloc(sim->dim,sizeof(double));
		for(d=0;d<sim->dim;d++){
			rxn->prdpos[k][d]=0;}}
	RxnSetCmpt(rxn,cmpt);
	RxnSetSurface(rxn,srf);
	rxnsetcondition(sim,-1,SClists,0);
	surfsetcondition(sim->srfss,SClists,0);
	rxn->rate=flt1;

	return;
}


/* RxnAddReaction_cplx */
rxnptr RxnAddReaction_cplx(simptr sim, char *rname, int molec_num, int order, int nprod, rct_mptr rct1, rct_mptr rct2, prd_mptr prd1, prd_mptr prd2, compartptr cmpt, surfaceptr srf, double flt1){
	char **newrname;
	rxnptr *newrxn;
	int *newtable,*newtable1;
	rxnssptr rxnss;
	rxnptr rxn;
	int indx,maxrxn,maxspecies,i,j,s,r,ir,sr,jr,rct,done,freerxn,k,k0,k1,entry,entry1,entry2;
	gpointer entry_ptr;

	rxnss=NULL;
	rxn=NULL;
	newrname=NULL;
	newrxn=NULL;
	maxrxn=0;
	freerxn=1;


	if(!sim->rxnss[molec_num]){													// allocate reaction superstructure, if needed
		CHECKS(sim->mols,"Cannot add reaction because no molecules defined");
		CHECKMEM(sim->rxnss[molec_num]=rxnssalloc(NULL,molec_num,sim->mols->maxspecies,sim->mols->maxsitecode));
		sim->rxnss[molec_num]->sim=sim;
		rxnsetcondition(sim,order,SCinit,0);
		rxnsetcondition(sim,-1,SClists,0);
	}
	rxnss=sim->rxnss[molec_num];
	maxspecies=rxnss->maxspecies;

	CHECKMEM(rxn=rxnalloc(molec_num));						
	if(molec_num==1){															// key_num=1
		for(i=0;i<rct1->states_num;i++){ 
			entry=g_pairing(rct1->ident,rct1->states[i]);
			g_hash_table_insert(rxnss->table,GINT_TO_POINTER(entry),g_slist_append((GSList*)g_hash_table_lookup(rxnss->table,GINT_TO_POINTER(entry)),GINT_TO_POINTER(rxnss->totrxn)));
			entry_ptr=g_hash_table_lookup(rxnss->table,GINT_TO_POINTER(entry));
			g_hash_table_insert(rxnss->entrylist,entry_ptr,GINT_TO_POINTER(g_slist_length((GSList*)entry_ptr)));
			printf("RxnAdd, entrylist_size=%d, molec_num=%d, ident=%d, states=%d, entry=%d, entry_ptr=%d\n", g_hash_table_size(rxnss->entrylist), molec_num, rct1->ident, rct1->states[i], entry,entry_ptr);
			// if(g_slist_find(rxnss->entrylist,entry_ptr)==NULL)
		 	// 	rxnss->entrylist=g_slist_append(rxnss->entrylist,entry_ptr);	
	}}
	

	if(molec_num==2){
		for(i=0;i<rct1->states_num;i++){
			printf("molec_num=%d, rct1->states_num=%d, i=%d\n", molec_num, rct1->states_num, i);
			entry1=g_pairing(rct1->ident,rct1->states[i]);
			for(j=0;j<rct2->states_num;j++) {
				entry2=g_pairing(rct2->ident,rct2->states[j]);		
				entry=g_pairing(entry1,entry2);
				g_hash_table_insert(rxnss->table,GINT_TO_POINTER(entry),g_slist_append((GSList*)g_hash_table_lookup(rxnss->table,GINT_TO_POINTER(entry)),GINT_TO_POINTER(rxnss->totrxn)));
				entry_ptr=g_hash_table_lookup(rxnss->table,GINT_TO_POINTER(entry));
				g_hash_table_insert(rxnss->entrylist,entry_ptr,GINT_TO_POINTER(g_slist_length((GSList*)entry_ptr)));
				printf("RxnAdd, entrylist_size=%d, molec_num=%d, rct1->ident=%d, rct1->states[i]=%d, rct2->ident=%d, rct2->states[j]=%d, entry1=%d, entry2=%d, entry=%d, entry_ptr=%d\n", g_hash_table_size(rxnss->entrylist), molec_num, rct1->ident, rct1->states[i], rct2->ident, rct2->states[j], entry1, entry2, entry, entry_ptr);
				// if(g_slist_find(rxnss->entrylist,entry_ptr)==NULL)
				//	rxnss->entrylist=g_slist_append(rxnss->entrylist,entry_ptr);
		}}
		rxnss->binding[rct1->ident][rct2->ident]=1;
		rxnss->binding[rct2->ident][rct1->ident]=1;
	}
			
	rxnsetup(sim,rxnss,rxn,rname,order,molec_num,nprod,rct1,rct2,prd1,prd2,cmpt,srf,flt1);	
	if(rxnss->totrxn+1>=rxnss->maxrxn){							
		rxnss->totrxn++;
		maxrxn=rxnss->totrxn;
		
		CHECKMEM(newrname=(char**)calloc(rxnss->totrxn,sizeof(char*)));
		for(r=0;r<rxnss->maxrxn;r++) newrname[r]=rxnss->rname[r];
		for(r=rxnss->maxrxn;r<maxrxn;r++) newrname[r]=NULL;
		for(r=rxnss->maxrxn;r<maxrxn;r++) CHECK(newrname[r]=EmptyString());
		
		CHECKMEM(newrxn=(rxnptr*)calloc(maxrxn,sizeof(rxnptr)));
		for(r=0;r<rxnss->maxrxn;r++) newrxn[r]=rxnss->rxn[r];
		for(r=rxnss->maxrxn;r<maxrxn;r++) newrxn[r]=NULL;

		strncpy(newrname[maxrxn-1],rname,STRCHAR-1);
		newrname[maxrxn-1][STRCHAR-1]='\0';

		if(rxnss->rxn) free(rxnss->rxn);
		rxnss->rxn=newrxn;
		newrxn=NULL; 
		if(rxnss->rname){
			free(rxnss->rname);
		}
		rxnss->rname=newrname;
		newrname=NULL;
		rxnss->maxrxn=maxrxn;
	}
	rxnss->rxn[rxnss->totrxn-1]=rxn;
	freerxn=0;

 failure:
	if(!rxnss) return NULL;
	if(newrname) {
		for(r=rxnss->maxrxn;r<maxrxn;r++) free(newrname[r]);
		free(newrname); }
	free(newrxn);
	if(freerxn) rxnfree(rxn);
	if(ErrorType==2) simLog(sim,8,"%s",ErrorString);
	else simLog(sim,10,"%s",ErrorString);
	return NULL; 
}

/*
// sort rxnss->entrylist and make it a circular linked list
rxnptr RxnAddReactionCheck(simptr sim,char *rname,int molec_num,int order,rct,int *sites,int *sites_val,enum MolecState *rctstate,int nprod,enum MolecState *prdstate,compartptr cmpt,surfaceptr srf) {
	rxnptr rxn;
	int i;

	CHECKBUG(sim,"sim undefined");
	CHECKBUG(sim->mols,"sim is missing molecule superstructure");
	CHECKBUG(rname,"rname is missing");
	CHECKBUG(strlen(rname)<STRCHAR,"rname is too long");
	CHECKBUG(order>=0 && order<=2,"order is out of bounds");
	if(order>0) {
		CHECKBUG(ident,"ident is missing"); }
	for(i=0;i<order;i++) {
		// CHECKBUG(rctident[i]>0 && rctident[i]<sim->mols->nspecies,"reactant identity out of bounds");
		CHECKBUG(rctstate[i]>=0 && rctstate[i]<MSMAX1,"reactant state out of bounds"); }
	CHECKBUG(nprod>=0,"nprod out of bounds");
	for(i=0;i<nprod;i++) {
		// CHECKBUG(prdident[i]>0 && prdident[i]<sim->mols->nspecies,"reactant identity out of bounds");
		CHECKBUG(prdstate[i]>=0 && prdstate[i]<MSMAX1,"reactant state out of bounds"); }
	if(cmpt) {
		CHECKBUG(sim->cmptss,"sim is missing compartment superstructure"); }
	if(srf)	{
		CHECKBUG(sim->srfss,"sim is missing surface superstructure"); }
	// rxn=RxnAddReaction_cplx(sim,rname,order,rctident,rctstate,nprod,prdident,prdstate,cmpt,srf);	
	
	// molec_num=(nprod>order)?nprod:order;
	for(i=0;i<molec_num;i++){
		CHECKBUG(ident[i]>0 && ident[i]<sim->mols->nspecies, "molecule identity out of bounds");
		CHECKBUG(sites[i]>=0 && sites[i]<=sim->mols->spsites_num[ident[i]], "binding site identity out of bounds");
		CHECKBUG(sites_val[i]==0 || sites_val[i]==1, "invalid site states");
	}	

	// RxnAddReaction_cplx(sim,rname,order,nprod,ident,rctstate,prdstate,sites,sites_val,rxncond1,rxncond2,cmpt,srf);
	// RxnAddReaction_cplx(sim,rname,molec_num,order,nprod,ident,rctstate,prdstate,sites,sites_val,NULL,NULL,NULL,NULL,cmpt,srf);
	return rxn;
failure:
	simLog(sim,10,"%s",ErrorString);
	return NULL; }
*/


/* rxnreadstring */
rxnssptr rxnreadstring(simptr sim,ParseFilePtr pfp,rxnssptr rxnss,char *word,char *line2) {
	int order,maxspecies,itct;
	char nm[STRCHAR],nm2[STRCHAR],rxnnm[STRCHAR];
	int i,r,prd,j,i1,i2,i3,nptemp,identlist[MAXPRODUCT],d;
	double rtemp,postemp[DIMMAX];
	enum MolecState ms,ms1,ms2,mslist[MAXPRODUCT];
	rxnptr rxn;
	enum RevParam rparamt;
	
	order=-1;
	maxspecies=sim->mols->maxspecies;
	
	/*
	if(!strcmp(word,"order")) {							// order
		itct=sscanf(line2,"%i",&order);
		CHECKS(itct==1,"error reading order");
		CHECKS(order>=0 && order<=2,"order needs to be between 0 and 2");
		if(!sim->rxnss[order]) {
			CHECKS(sim->rxnss[order]=rxnssalloc(NULL,order,maxspecies,sim->mols->maxsitecode),"out of memory creating reaction superstructure");
			rxnsetcondition(sim,order,SCinit,0);
			sim->rxnss[order]->sim=sim; }
		rxnss=sim->rxnss[order];
		CHECKS(!strnword(line2,2),"unexpected text following order"); }
	*/

	if(!strcmp(word,"max_rxn")) {						// max_rxn
		}
	
	else if(!strcmp(word,"reactant") && order==0) {	// reactant, 0
		CHECKS(order>=0,"order needs to be entered before reactant");
		j=wordcount(line2);
		CHECKS(j>0,"number of reactions needs to be >0");
		for(j--;j>=0;j--) {
			itct=sscanf(line2,"%s",nm);
			CHECKS(itct==1,"missing reaction name in reactant");
			CHECKS(stringfind(rxnss->rname,rxnss->totrxn,nm)<0,"reaction name has already been used");
			CHECKS(RxnAddReaction(sim,nm,0,NULL,NULL,0,NULL,NULL,NULL,NULL),"faied to add 0th order reaction");
			line2=strnword(line2,2); }}
	
	else if(!strcmp(word,"reactant") && order==1) {	// reactant, 1
		CHECKS(order>=0,"order needs to be entered before reactant");
		i=readmolname(sim,line2,&ms,0);
		CHECKS(i!=0,"empty molecules cannot react");
		CHECKS(i!=-1,"reactant format: name[(state)] rxn_name");
		CHECKS(i!=-2,"mismatched or improper parentheses around molecule state");
		CHECKS(i!=-3,"cannot read molecule state value");
		CHECKS(i!=-4,"molecule name not recognized");
		CHECKS(i!=-5,"molecule name cannot be set to 'all'");
		CHECKS(i!=-6,"molecule name cannot include wildcards");
		CHECKS(ms!=MSbsoln,"bsoln is not an allowed state for first order reactants");
		identlist[0]=i;
		mslist[0]=ms;
		CHECKS(line2=strnword(line2,2),"no reactions listed");
		j=wordcount(line2);
		for(j--;j>=0;j--) {
			itct=sscanf(line2,"%s",nm);
			CHECKS(itct==1,"missing reaction name in reactant");
			CHECKS(stringfind(rxnss->rname,rxnss->totrxn,nm)<0,"reaction name has already been used");
			CHECKS(RxnAddReaction(sim,nm,1,identlist,mslist,0,NULL,NULL,NULL,NULL),"faied to add 1st order reaction");
			line2=strnword(line2,2); }}
	
	else if(!strcmp(word,"reactant") && order==2) {	// reactant, 2
		CHECKS(order>=0,"order needs to be entered before reactants");
		i1=readmolname(sim,line2,&ms1,0);
		CHECKS(i1!=0,"empty molecules cannot react");
		CHECKS(i1!=-1,"reactant format: name[(state)] + name[(state)] rxn_name");
		CHECKS(i1!=-2,"mismatched or improper parentheses around molecule state");
		CHECKS(i1!=-3,"cannot read molecule state value");
		CHECKS(i1!=-4,"molecule name not recognized");
		CHECKS(i1!=-5,"molecule name cannot be set to 'all'");
		CHECKS(i1!=-6,"molecule name cannot include wildcards");
		identlist[0]=i1;
		mslist[0]=ms1;
		CHECKS(line2=strnword(line2,3),"reactant format: name[(state)] + name[(state)] rxn_list");
		i2=readmolname(sim,line2,&ms2,0);
		CHECKS(i2!=0,"empty molecules cannot react");
		CHECKS(i2!=-1,"reactant format: name[(state)] + name[(state)] rxn_name value");
		CHECKS(i2!=-2,"mismatched or improper parentheses around molecule state");
		CHECKS(i2!=-3,"cannot read molecule state value");
		CHECKS(i2!=-4,"molecule name not recognized");
		CHECKS(i2!=-5,"molecule name cannot be set to 'all'");
		CHECKS(i2!=-6,"molecule name cannot include wildcards");
		identlist[1]=i2;
		mslist[1]=ms2;
		CHECKS(line2=strnword(line2,2),"no reactions listed");
		j=wordcount(line2);
		for(j--;j>=0;j--) {
			itct=sscanf(line2,"%s",nm);
			CHECKS(itct==1,"missing reaction name in reactant");
			CHECKS(stringfind(rxnss->rname,rxnss->totrxn,nm)<0,"reaction name has already been used");
			CHECKS(RxnAddReaction(sim,nm,2,identlist,mslist,0,NULL,NULL,NULL,NULL),"faied to add 1st order reaction");
			line2=strnword(line2,2); }}
	
	else if(!strcmp(word,"permit") && order==0) {		// permit, 0
		CHECKS(0,"reaction permissions are not allowed for order 0 reactions"); }
	
	else if(!strcmp(word,"permit") && order==1) {		// permit, 1
		CHECKS(order>=0,"order needs to be entered before permit");
		i=readmolname(sim,line2,&ms,0);
		CHECKS(i!=0,"empty molecules cannot be entered");
		CHECKS(i!=-1,"permit format: name(state) rxn_name value");
		CHECKS(i!=-2,"mismatched or improper parentheses around molecule state");
		CHECKS(i!=-3,"cannot read molecule state value");
		CHECKS(i!=-4,"molecule name not recognized");
		CHECKS(i!=-5,"molecule name cannot be set to 'all'");
		CHECKS(i!=-6,"molecule name cannot include wildcards");
		CHECKS(ms<MSMAX,"all and bsoln are not allowed in permit for first order reactions");
		CHECKS(line2=strnword(line2,2),"permit format: name(state) rxn_name value");
		itct=sscanf(line2,"%s %i",rxnnm,&i3);
		CHECKS(itct==2,"permit format: name(state) rxn_name value");
		r=stringfind(rxnss->rname,rxnss->totrxn,rxnnm);
		CHECKS(r>=0,"in permit, reaction name not recognized");
		// for(j=0;j<rxnss->nrxn[i] && rxnss->table[i][j]!=r;j++);
		// CHECKS(rxnss->table[i][j]==r,"in permit, reaction was not already listed for this reactant");
		CHECKS(i3==0 || i3==1,"in permit, value needs to be 0 or 1");
		rxnss->rxn[r]->permit[ms]=i3;
		CHECKS(!strnword(line2,3),"unexpected text following permit"); }
	
	else if(!strcmp(word,"permit") && order==2) {		// permit, 2
		CHECKS(order>=0,"order needs to be entered before permit");
		i1=readmolname(sim,line2,&ms,0);
		CHECKS(i1!=0,"empty molecules not allowed");
		CHECKS(i1!=-1,"permit format: name(state) + name(state) rxn_name value");
		CHECKS(i1!=-2,"mismatched or improper parentheses around first molecule state");
		CHECKS(i1!=-3,"cannot read first molecule state value");
		CHECKS(i1!=-4,"first molecule name not recognized");
		CHECKS(i1!=-5,"first molecule state missing, or is set to 'all'");
		CHECKS(i1!=-6,"molecule name cannot include wildcards");
		CHECKS(ms<MSMAX1,"all is not allowed in permit");
		CHECKS(line2=strnword(line2,3),"permit format: name(state) + name(state) rxn_name value");
		i2=readmolname(sim,line2,&ms2,0);
		CHECKS(i2!=0,"empty molecules are not allowed");
		CHECKS(i2!=-1,"permit format: name(state) + name(state) rxn_name value");
		CHECKS(i2!=-2,"mismatched or improper parentheses around second molecule state");
		CHECKS(i2!=-3,"cannot read second molecule state value");
		CHECKS(i2!=-4,"second molecule name not recognized");
		CHECKS(i2!=-5,"second molecule state missing, or is set to 'all'");
		CHECKS(i2!=-6,"molecule name cannot include wildcards");
		CHECKS(ms2<MSMAX1,"all is not allowed in permit");
		CHECKS(line2=strnword(line2,2),"permit format: name(state) + name(state) rxn_name value");
		i=i1*maxspecies+i2;
		itct=sscanf(line2,"%s %i",rxnnm,&i3);
		CHECKS(itct==2,"permit format: name(state) + name(state) rxn_name value");
		r=stringfind(rxnss->rname,rxnss->totrxn,rxnnm);
		CHECKS(r>=0,"in permit, reaction name not recognized");
		// for(j=0;j<rxnss->nrxn[i] && rxnss->table[i][j]!=r;j++);
		// CHECKS(rxnss->table[i][j]==r,"in permit, reaction was not already listed for this reactant");
		CHECKS(i3==0 || i3==1,"in permit, value needs to be 0 or 1");
		rxnss->rxn[r]->permit[ms*MSMAX1+ms2]=i3;
		CHECKS(!strnword(line2,3),"unexpected text following permit"); }
	
	else if(!strcmp(word,"rate")) {								// rate
		CHECKS(order>=0,"order needs to be entered before rate");
		itct=sscanf(line2,"%s %lg",nm,&rtemp);
		CHECKS(itct==2,"format for rate: rxn_name rate");
		r=stringfind(rxnss->rname,rxnss->totrxn,nm);
		CHECKS(r>=0,"unknown reaction name in rate");
		CHECKS(rtemp>=0,"reaction rate needs to be >=0 (maybe try rate_internal)");
		rxnss->rxn[r]->rate=rtemp;
		CHECKS(!strnword(line2,3),"unexpected text following rate"); }
	
	/*
	else if(!strcmp(word,"confspread_radius")) {	// confspread_radius
		CHECKS(order>=0,"order needs to be entered before confspread_radius");
		itct=sscanf(line2,"%s %lg",nm,&rtemp);
		CHECKS(itct==2,"format for confspread_radius: rxn_name radius");
		r=stringfind(rxnss->rname,rxnss->totrxn,nm);
		CHECKS(r>=0,"unknown reaction name in confspread_radius");
		CHECKS(rxnss->rxn[r]->rparamt!=RPconfspread,"confspread_radius can only be entered once for a reaction");
		CHECKS(rtemp>=0,"confspread_radius needs to be >=0");
		rxnss->rxn[r]->bindrad2=rtemp*rtemp;
		rxnss->rxn[r]->rparamt=RPconfspread;
		CHECKS(!strnword(line2,3),"unexpected text following confspread_radius"); }	

	else if(!strcmp(word,"rate_internal")) {			// rate_internal
		CHECKS(order>=0,"order needs to be entered before rate_internal");
		itct=sscanf(line2,"%s %lg",nm,&rtemp);
		CHECKS(itct==2,"format for rate_internal: rxn_name rate");
		r=stringfind(rxnss->rname,rxnss->totrxn,nm);
		CHECKS(r>=0,"unknown reaction name in rate_internal");
		CHECKS(rtemp>=0,"rate_internal needs to be >=0");
		if(order<2) rxnss->rxn[r]->prob=rtemp;
		else rxnss->rxn[r]->bindrad2=rtemp*rtemp;
		CHECKS(!strnword(line2,3),"unexpected text following rate_internal"); }
	*/	

	else if(!strcmp(word,"probability")) {			// probability
		CHECKS(order>=0,"order needs to be entered before probability");
		itct=sscanf(line2,"%s %lg",nm,&rtemp);
		CHECKS(itct==2,"format for probability: rxn_name probability");
		r=stringfind(rxnss->rname,rxnss->totrxn,nm);
		CHECKS(r>=0,"unknown reaction name in probability");
		CHECKS(rtemp>=0,"probability needs to be >=0");
		CHECKS(rtemp<=1,"probability needs to be <=1");
		rxnss->rxn[r]->prob=rtemp;
		CHECKS(!strnword(line2,3),"unexpected text following probability"); }
	
	else if(!strcmp(word,"product")) {						// product
		CHECKS(order>=0,"order needs to be entered before product");
		itct=sscanf(line2,"%s",rxnnm);
		CHECKS(itct==1,"format for product: rxn_name product_list");
		r=stringfind(rxnss->rname,rxnss->totrxn,rxnnm);
		CHECKS(r>=0,"unknown reaction name in product");
		nptemp=symbolcount(line2,'+')+1;
		CHECKS(nptemp>=0,"number of products needs to be >=0");
		CHECKS(nptemp<=MAXPRODUCT,"more products are entered than Smoldyn can handle");
		CHECKS(rxnss->rxn[r]->nprod==0,"products for a reaction can only be entered once");
		for(prd=0;prd<nptemp;prd++) {
			CHECKS(line2=strnword(line2,2),"product list is incomplete");
			i=readmolname(sim,line2,&ms,0);
			CHECKS(i!=0,"empty molecules cannot be products");
			CHECKS(i!=-1,"product format: rxn_name name(state) + name(state) + ...");
			CHECKS(i!=-2,"mismatched or improper parentheses around molecule state");
			CHECKS(i!=-3,"cannot read molecule state value");
			CHECKS(i!=-4,"molecule name not recognized");
			CHECKS(i!=-5,"molecule name cannot be 'all'");
			CHECKS(i!=-6,"molecule name cannot include wildcards");
			CHECKS(ms<MSMAX1,"product state is not allowed");
			identlist[prd]=i;
			mslist[prd]=ms;
			if(prd+1<nptemp) {
				CHECKS(line2=strnword(line2,2),"incomplete product list"); }}
		CHECKS(RxnAddReaction(sim,rxnnm,order,NULL,NULL,nptemp,identlist,mslist,NULL,NULL),"failed to add products to reaction");
		CHECKS(!strnword(line2,2),"unexpected text following product"); }
	
	else if(!strcmp(word,"product_param")) {				// product_param
		CHECKS(order>=0,"order needs to be entered before product_param");
		itct=sscanf(line2,"%s",nm);
		CHECKS(itct==1,"format for product_param: rxn type [parameters]");
		r=stringfind(rxnss->rname,rxnss->totrxn,nm);
		CHECKS(r>=0,"unknown reaction name in product_param");
		rxn=rxnss->rxn[r];
		rparamt=rxn->rparamt;
		CHECKS(rparamt==RPnone,"product_param can only be entered once");
		CHECKS(line2=strnword(line2,2),"format for product_param: rxn type [parameters]");
		itct=sscanf(line2,"%s",nm);
		CHECKS(itct==1,"missing parameter type in product_param");
		rparamt=rxnstring2rp(nm);
		CHECKS(rparamt!=RPnone,"unrecognized parameter type");
		rtemp=0;
		prd=0;
		for(d=0;d<sim->dim;d++) postemp[prd]=0;
		if(rparamt==RPpgem || rparamt==RPpgemmax || rparamt==RPratio || rparamt==RPunbindrad || rparamt==RPpgem2 || rparamt==RPpgemmax2 || rparamt==RPratio2) {
			CHECKS(line2=strnword(line2,2),"missing parameter in product_param");
			itct=sscanf(line2,"%lg",&rtemp);
			CHECKS(itct==1,"error reading parameter in product_param"); }
		else if(rparamt==RPoffset || rparamt==RPfixed) {
			CHECKS(line2=strnword(line2,2),"missing parameters in product_param");
			itct=sscanf(line2,"%s",nm2);
			CHECKS(itct==1,"format for product_param: rxn type [parameters]");
				CHECKS((i=stringfind(sim->mols->spname,sim->mols->nspecies,nm2))>=0,"unknown molecule in product_param");
				// for(prd=0;prd<rxn->nprod && rxn->prdident[prd]!=i;prd++);
				for(prd=0;prd<rxn->nprod && rxn->prd[prd]->ident!=i;prd++);
				CHECKS(prd<rxn->nprod,"molecule in product_param is not a product of this reaction");
				CHECKS(line2=strnword(line2,2),"position vector missing for product_param");
				itct=strreadnd(line2,sim->dim,postemp,NULL);
				CHECKS(itct==sim->dim,"insufficient data for position vector for product_param");
				line2=strnword(line2,sim->dim); }
			i1=RxnSetRevparam(sim,rxn,rparamt,rtemp,prd,postemp,sim->dim);
			CHECKS(i1!=1,"reversible parameter type can only be set once");
		CHECKS(i1!=2,"reversible parameter value is out of bounds");
		CHECKS(!strnword(line2,2),"unexpected text following product_param"); }
	
	else {																				// unknown word
		CHECKS(0,"syntax error within reaction block: statement not recognized"); }

	return rxnss;
	
 failure:
	simParseError(sim,pfp);
	return NULL; }


/* loadrxn */
int loadrxn(simptr sim,ParseFilePtr *pfpptr,char *line2) {
	ParseFilePtr pfp;
	char word[STRCHAR],errstring[STRCHAR];
	int done,pfpcode,firstline2;
	rxnssptr rxnss;

	pfp=*pfpptr;
	done=0;
	rxnss=NULL;
	firstline2=line2?1:0;

	while(!done) {
		if(pfp->lctr==0)
			simLog(sim,2," Reading file: '%s'\n",pfp->fname);
		if(firstline2) {
			strcpy(word,"order");
			pfpcode=1;
			firstline2=0; }
		else
			pfpcode=Parse_ReadLine(&pfp,word,&line2,errstring);
		*pfpptr=pfp;
		CHECKS(pfpcode!=3,"%s",errstring);

		if(pfpcode==0);																// already taken care of
		else if(pfpcode==2) {													// end reading
			done=1; }
		else if(!strcmp(word,"end_reaction")) {				// end_reaction
			CHECKS(!line2,"unexpected text following end_reaction");
			return 0; }
		else if(!line2) {															// just word
			CHECKS(0,"unknown word or missing parameter"); }
		else {
			rxnss=rxnreadstring(sim,pfp,rxnss,word,line2);
			CHECK(rxnss); }}

	CHECKS(0,"end of file encountered before end_reaction statement");	// end of file

 failure:																					// failure
	if(ErrorType!=1) simParseError(sim,pfp);
	*pfpptr=pfp=NULL;
	return 1; }


/* rxnsupdateparams */
int rxnsupdateparams(simptr sim) {
	int er,wflag,k;
	char errorstr[STRCHAR];
	rxnssptr rxnss;
	
	wflag=strchr(sim->flags,'w')?1:0;
	for(k=0;k<MAXORDER;k++){
		rxnss=sim->rxnss[k];
		if(rxnss){
			er=rxnsetrates(sim,sim->rxnss[k]->molec_num,errorstr);							// set rates
			if(er>=0) {
				simLog(sim,8,"Error setting rate for reaction  %i, reaction %s\n%s\n",rxnss->molec_num,rxnss->rname[er],errorstr);
				return 3; }}}
	/*	
	for(k=0;k<MAXORDER;k++){
		rxnss=sim->rxnss[k];		
		if(rxnss){
			if(sim->rxnss[k] && sim->rxnss[k]->condition<=SCparams) {
				errorstr[0]='\0';
				er=rxnsetproducts(sim,rxnss->molec_num,errorstr);						// set products
				if(er>=0) {
					simLog(sim,8,"Error setting products for reaction  %i, reaction %s\n%s\n",rxnss->molec_num,rxnss->rname[er],errorstr);
					return 3; }
				if(!wflag && strlen(errorstr)) simLog(sim,5,"%s\n",errorstr); }}}
	*/
	
	for(k=0;k<MAXORDER;k++){												// calculate tau values
		rxnss=sim->rxnss[k];
		if(rxnss){
			if(sim->rxnss[k] && sim->rxnss[k]->condition<=SCparams)
				rxncalctau(sim,rxnss->molec_num);
		}}
	return 0; }

// don't think need this function, because all molecules are live
/* rxnsupdatelists */
/*
int rxnsupdatelists(simptr sim,int molec_num){
	rxnssptr rxnss;
	int maxlist,ll,nl2o,r,i1,i2,ll1,ll2;
	rxnptr rxn;
	enum MolecState ms1,ms2;

	rxnss=sim->rxnss[molec_num];
	if(!sim->mols || sim->mols->condition<SCparams) return 2;

	maxlist=rxnss->maxlist;								// set reaction molecule lists
	if(maxlist!=sim->mols->maxlist) {
		free(rxnss->rxnmollist);
		rxnss->rxnmollist=NULL;
		maxlist=sim->mols->maxlist;
		if(maxlist>0) {
			nl2o=intpower(maxlist,order);
			rxnss->rxnmollist=(int*) calloc(nl2o,sizeof(int));
			CHECKMEM(rxnss->rxnmollist); }
		rxnss->maxlist=maxlist; }

	if(maxlist>0) {
		nl2o=intpower(maxlist,order);
		for(ll=0;ll<nl2o;ll++) rxnss->rxnmollist[ll]=0;

		for(r=0;r<rxnss->totrxn;r++) {
			rxn=rxnss->rxn[r];
			// i1=rxn->rctident[0];
			i1=rxn->ident[0];
			if(order==1) {
				for(ms1=0;ms1<MSMAX1;ms1=ms1+1) {
#if OPTION_VCELL
					if(rxn->permit[ms1] && (rxn->prob>0 || rxn->rate>0 || rxn->rateValueProvider != NULL))
#else
					if(rxn->permit[ms1] && (rxn->prob>0 || rxn->rate>0))
#endif
						{
							ll1=sim->mols->listlookup[i1][ms1];
							rxnss->rxnmollist[ll1]=1; }
				}}
			else if(order==2) {
				// i2=rxn->rctident[1];
				i2=rxn->ident[1];
				for(ms1=0;ms1<MSMAX1;ms1=ms1+1)
					for(ms2=0;ms2<MSMAX1;ms2=ms2+1) {

#if OPTION_VCELL
						if(rxn->permit[ms1*MSMAX1+ms2] && rxn->prob!=0 && (rxn->rate>0 || rxn->bindrad2>0 || rxn->rateValueProvider != NULL))
#else
						if(rxn->permit[ms1*MSMAX1+ms2] && rxn->prob!=0 && (rxn->rate>0 || rxn->bindrad2>0))
#endif
							{
								ll1=sim->mols->listlookup[i1][ms1==MSbsoln?MSsoln:ms1];
								ll2=sim->mols->listlookup[i2][ms2==MSbsoln?MSsoln:ms2];
								rxnss->rxnmollist[ll1*maxlist+ll2]=1;
								rxnss->rxnmollist[ll2*maxlist+ll1]=1; }

					}}}}

	return 0;
failure:
	simLog(sim,10,"Unable to allocate memory in rxnsupdatelists");
	return 1; }
*/


/* rxnsupdate */
int rxnsupdate(simptr sim) {
	int er,doparams,k;

	for(k=0;k<MAXORDER;k++) {
		if(sim->rxnss[k] && sim->rxnss[k]->condition<=SClists) {
			// er=rxnsupdatelists(sim,order);
			// if(er) return er;
			rxnsetcondition(sim,sim->rxnss[k]->molec_num,SCparams,1); }}

	doparams=0;
	for(k=0;k<MAXORDER;k++){
		if(sim->rxnss[k] && sim->rxnss[k]->condition<SCok) doparams=1; }
	if(doparams) {
		er=rxnsupdateparams(sim);
		if(er) return er;
		rxnsetcondition(sim,-1,SCok,1); }

	return 0; }



/******************************************************************************/
/************************** core simulation functions *************************/
/******************************************************************************/
// removed the parts on rxn parameters about bounce, confpread, etc
/* doreact */
int doreact(rxnssptr rxnss,gpointer rptr,moleculeptr mptr1,moleculeptr mptr2,int ll1,int m1,int ll2,int m2,double *pos,panelptr pnl,int rxn_site_indx1,int rxn_site_indx2,double r,double dc1,double dc2) {
	int i, k,order,prd,d,nprod,dim,calc,dorxnlog,prd2, d1, d2,s, i1, i2;
	long int serno,sernolist[MAXPRODUCT];
	double x,dist;
	molssptr mols;
	moleculeptr mptr,mptrallo,mptr_tmp, dif_molec1, dif_molec2, cplx_dif;
	boxptr rxnbptr;
	double v2[DIMMAX],v1[DIMMAX],rxnpos[DIMMAX],m3[DIMMAX*DIMMAX];
	enum MolecState ms;
	double delta_dist;
	gpointer entry1_ptr,entry2_ptr;
	rxnptr rxn;
	simptr sim;
	int site_indx_tmp;
	char* site_name_tmp;
	
	sim=rxnss->sim;
	rxn=rxnss->rxn[(int)(intptr_t)((GSList*)rptr)->data];
	mols=sim->mols;
	dim=sim->dim;
	order=rxn->order;
	dorxnlog=0;
	
	double offset1[dim], offset2[dim]; // pos_tmp1, pos_tmp2;
	for(d=0;d<dim;d++) offset1[d]=offset2[d]=0;

// get reaction position in rxnpos, pnl, rxnbptr
	if(order==0) {															// order 0
		for(d=0;d<dim;d++) rxnpos[d]=pos[d];
		rxnbptr=pos2box(sim,rxnpos); }

	else if(order==1) {														// order 1
		for(d=0;d<dim;d++) rxnpos[d]=mptr1->pos[d];
		pnl=mptr1->pnl;
		rxnbptr=mptr1->box;		
	}

	else if(order==2) {														// order 2
		if(mptr1->ident==2 && mptr2->ident==2) 
			asm("");
		i1=difmolec(sim, mptr1,&dif_molec1);
		dc1=mols->difc[i1][dif_molec1->mstate];
		i2=difmolec(sim, mptr2,&dif_molec2);
		dc2=mols->difc[i2][dif_molec2->mstate];
		//if(sim->events)
		//	fprintf(sim->events,"\nmptr1->serno=%d mptr2->serno=%d dif_molec1->serno=%d pos[2]=%f dif_molec2->serno=%d pos[2]=%f dc1=%f dc2=%f\n", mptr1->serno, mptr2->serno, dif_molec1->serno, dif_molec1->pos[2],dif_molec2->serno, dif_molec2->pos[2],dc1,dc2);
		if(dc1==0 && dc2==0) x=0.5;
		else x=dc2/(dc1+dc2);

		//for(d=0;d<dim;d++) rxnpos[d]=x*dif_molec1->pos[d]+(1.0-x)*dif_molec2->pos[d];
		if(rxn_site_indx1>=0 && rxn_site_indx2>=0)
			for(d=0;d<dim;d++) rxnpos[d]=x*mptr1->pos[d]+(1.0-x)*mptr2->pos[d];
		
		if(mptr1->pnl || mptr2->pnl){
			if(mptr1->pnl && ptinpanel(rxnpos,mptr1->pnl,dim)) pnl=mptr1->pnl;
			else if(mptr2->pnl && ptinpanel(rxnpos,mptr2->pnl,dim)) pnl=mptr2->pnl;
			else if(mptr1->pnl) {
				pnl=mptr1->pnl;
				for(d=0;d<dim;d++) rxnpos[d]=mptr1->pos[d]; }
			else {
				pnl=mptr2->pnl;
				for(d=0;d<dim;d++) rxnpos[d]=mptr2->pos[d]; }}
		else pnl=NULL;
		rxnbptr=mptr1->box;
	}
	else { return 0; }														// order > 2

	if(rxn->logserno) {														// determine if logging needed
		if(rxn->logserno->n==0) dorxnlog=1;
		else if(mptr1 && List_MemberLI(rxn->logserno,mptr1->serno)) dorxnlog=1;
		else if(mptr2 && List_MemberLI(rxn->logserno,mptr2->serno)) dorxnlog=1;
		else dorxnlog=0; 
	}

// place products //why need to recalulate the place?	
	nprod=rxn->nprod;
	prd=nprod-1;
	calc=0;
	dist=0;
	
	int new_mol=nprod-order, molec_gen;
	siteptr site1, site2;

	mptr1->ident=rxn->prd[0]->ident;
	if(mptr2)
		mptr2->ident=rxn->prd[1]->ident;

	if(new_mol>0){
		if(!mptr2){
			molec_gen=1;
			mptr2=getnextmol_cplx(mols,1,rxn->prd[1]->ident);			
			mptr2->list=sim->mols->listlookup[mptr2->ident][mptr2->mstate];
			mptr2->box=rxnbptr;
		}
		else 
			molec_gen=0;

		for(i=0;i<rxn->prd[0]->sites_num;i++){		
			mptr1->sites[rxn->prd[0]->sites_indx[i]]->value[0]=rxn->prd[0]->sites_val[i];
			mptr1->sites[rxn->prd[0]->sites_indx[i]]->time=sim->time;
			if(mptr1->sites[rxn->prd[0]->sites_indx[i]]->site_type==2){
				site_name_tmp=sim->mols->spsites_name[rxn->prd[0]->sites_indx[i]][0];	
				site_indx_tmp=stringfind(sim->mols->spsites_name[mptr1->ident],sim->mols->spsites_num[mptr1->ident],site_name_tmp);
				mptr1->sites[rxn->prd[0]->sites_indx[i]]->bind->sites[site_indx_tmp]->bind=NULL;
				mptr1->sites[rxn->prd[0]->sites_indx[i]]->bind->sites[site_indx_tmp]->value[0]=0;
				mptr1->sites[rxn->prd[0]->sites_indx[i]]->bind=NULL;
				mptr1->sites[rxn->prd[0]->sites_indx[i]]->value[0]=0;
				//printf("time=%f, rxn_name=%s, enzyme_serno=%d\n", sim->time, rxn->rname, mptr1->sites[rxn->prd[0]->sites_indx[i]]->bind->serno);
			}

		}

		for(i=0;i<rxn->prd[1]->sites_num;i++){
			mptr2->sites[rxn->prd[1]->sites_indx[i]]->value[0]=rxn->prd[1]->sites_val[i];
			mptr2->sites[rxn->prd[1]->sites_indx[i]]->time=sim->time;
			if(mptr2->sites[rxn->prd[1]->sites_indx[i]]->site_type==2){
				site_name_tmp=sim->mols->spsites_name[rxn->prd[1]->sites_indx[i]][0];	
				site_indx_tmp=stringfind(sim->mols->spsites_name[mptr2->ident],sim->mols->spsites_num[mptr2->ident],site_name_tmp);
				mptr2->sites[rxn->prd[1]->sites_indx[i]]->bind->sites[site_indx_tmp]->bind=NULL;
				mptr2->sites[rxn->prd[1]->sites_indx[i]]->bind->sites[site_indx_tmp]->value[0]=0;
				mptr2->sites[rxn->prd[1]->sites_indx[i]]->bind=NULL;
				mptr2->sites[rxn->prd[1]->sites_indx[i]]->value[0]=0;
			}
		}

		mptr1->sites[rxn_site_indx1]->bind=NULL; // place these lines ahead of posptr_assign. e.g. to consider, bK-gK_ub
		mptr2->sites[rxn_site_indx2]->bind=NULL;
		posptr_assign(sim->mols,mptr1,NULL,rxn_site_indx1);
		posptr_assign(sim->mols,mptr2,NULL,rxn_site_indx2);
		/*	
		if(mptr1->ident==mptr2->ident){
			mptr1->from=mptr1->to=NULL;
			mptr2->from=mptr2->to=NULL;
		//	mptr1->tot_sunit=mptr2->tot_sunit=1;
		//	mptr2->s_index=0;
		}
		*/
		i1=difmolec(sim, mptr1, &dif_molec1);
		i2=difmolec(sim, mptr2, &dif_molec2);
		
		if(!r){
			r=radius(sim,rptr,mptr1,mptr2,&dc1,&dc2,molec_gen);
			if(sim->events)
				fprintf(sim->events, "\ndif_molec1->serno=%d pos[2]=%f dif_molec2->serno=%d pos[2]=%f i1=%d dc1=%f i2=%d dc2=%f\n", dif_molec1->serno,dif_molec1->pos[2],dif_molec2->serno,dif_molec2->pos[2],i1,dc1,i2,dc2);
		}
	
		if(dc1==0 && dc2==0) x=0.5;
		else x=dc2/(dc2+dc1);
		if(r>0){
			rxn->prdpos[0][0]=r*(1-x);
			rxn->prdpos[1][0]=-r*x;
		}
		for(d=0;d<dim;d++)	mptr2->posx[d]=mptr1->posx[d];
		mptr2->mstate=ms=rxn->prd[prd]->prdstate;
		mptr2->pnl=pnl;
		if(!pnl);													// soln -> soln
		else if(ms==MSsoln){										// surf -> front soln
			mptr2->pnl=NULL;
			fixpt2panel(mptr2->posx,pnl,dim,PFfront,sim->srfss->epsilon); 	}
		else if(ms==MSbsoln){										// surf -> back soln
			mptr2->mstate=MSsoln;
			mptr2->pnl=NULL;
			fixpt2panel(mptr2->posx,pnl,dim,PFback,sim->srfss->epsilon); 	}
		else if(ms==MSfront){										// surf -> front surf
			fixpt2panel(mptr2->posx,pnl,dim,PFfront,sim->srfss->epsilon); 	}
		else if(ms==MSback){										// surf -> back surf
			fixpt2panel(mptr2->posx,pnl,dim,PFback,sim->srfss->epsilon); 	}
		else{														// surf -> surf: up, down
			fixpt2panel(mptr2->posx,pnl,dim,PFnone,sim->srfss->epsilon); 	}	

		for(d1=0;d1<dim && rxn->prdpos[0][d1]==0;d1++);
		for(d2=0;d2<dim && rxn->prdpos[1][d2]==0;d2++);

		if(d1!=dim || d2!=dim){
			// if(rxn->rparamt==RPfixed){ for(d=0;d<dim;d++) v1[d]=rxn->prdpos[prd][d]; }
			if(dim==1) {
				if(!calc) { m3[0]=signrand();calc=1; }
				v1[0]=m3[0]*rxn->prdpos[prd][0]; }
			else if(dim==2) {
				if(!calc) {DirCosM2D(m3,unirandCOD(0,2*PI));calc=1;}
				dotMVD(m3,rxn->prdpos[prd],v1,2,2); }
			else if(dim==3) {
				if(!calc) {DirCosMD(m3,thetarandCCD(),unirandCOD(0,2*PI),unirandCOD(0,2*PI));calc=1;}		// DirCosMD() in lib/Rn.c
				dotMVD(m3,rxn->prdpos[0],v1,3,3);
				if(!calc) {DirCosMD(m3,thetarandCCD(),unirandCOD(0,2*PI),unirandCOD(0,2*PI));calc=1;}		// DirCosMD() in lib/Rn.c
				dotMVD(m3,rxn->prdpos[1],v2,3,3);	
			} 														// dotMVD in lib/Rn.c
			else {
				if(!calc) {DirCosMD(m3,thetarandCCD(),unirandCOD(0,2*PI),unirandCOD(0,2*PI));calc=1;}
				dotMVD(m3,rxn->prdpos[prd],v1,3,3);	}
			for(d=0;d<dim;d++){
				mptr2->prev_pos[d]=mptr2->pos[d];
				mptr1->prev_pos[d]=mptr1->pos[d];
				// consider dc2=0, dc1=1000, x=0, check mptr2->pos[d]-mptr1->pos[d]
				mptr2->pos[d]+=(v2[d]-v1[d])*x;
				mptr1->pos[d]+=(v2[d]-v1[d])*(x-1);
				//mptr2->pos[d]=rxnpos[d]+v2[d];
				//mptr1->pos[d]=rxnpos[d]+v1[d];
				offset1[d]=mptr1->pos[d]-mptr1->prev_pos[d];
				offset2[d]=mptr2->pos[d]-mptr2->prev_pos[d];
			}
		}
		else { 
			for(d=0;d<dim;d++) {
				mptr2->prev_pos[d]=mptr2->pos[d];
				mptr1->pos[d]=mptr2->pos[d]=rxnpos[d];
				offset2[d]=mptr2->pos[d]-mptr2->prev_pos[d];
			}
		}

		// update complex connections
		if(mptr1->tot_sunit>1 && mptr2->tot_sunit>1){		
			CHECKS(syncpos(mols,mptr1,rxn_site_indx1,NULL)!=-1, "react.c");
			CHECKS(syncpos(mols,mptr2,rxn_site_indx2,NULL)!=-1, "react.c");
			entry1_ptr=GINT_TO_POINTER(g_pairing(mptr1->complex_id,mptr2->complex_id));
			entry2_ptr=GINT_TO_POINTER(g_pairing(mptr2->complex_id,mptr1->complex_id));
			if(g_hash_table_lookup(mols->complex_connect,entry1_ptr))
				g_hash_table_remove(mols->complex_connect,entry1_ptr);
			if(g_hash_table_lookup(mols->complex_connect,entry2_ptr))
				g_hash_table_remove(mols->complex_connect,entry2_ptr);

			if(offset1[0]!=0 && offset1[1]!=0 && offset1[2]!=0)
				CHECKS(complex_pos(sim,mptr1,"pos_line2517",&offset1[0],1)!=-1,"react.c");
			if(offset2[0]!=0 && offset2[1]!=0 && offset2[2]!=0)
				CHECKS(complex_pos(sim,mptr2,"pos_line2519",&offset2[0],1)!=-1,"react.c");
		}
		else if(mptr1->tot_sunit>1 && mptr2->tot_sunit==1){
			CHECKS(syncpos(mols,mptr1,rxn_site_indx1,NULL)!=-1,"react.c");
			if(offset1[0]!=0 && offset1[1]!=0 && offset1[2]!=0)
				CHECKS(complex_pos(sim,mptr1,"pos_line2685",&offset1[0],1)!=-1,"react.c");
			CHECKS(syncpos(mols,mptr2,rxn_site_indx2,&offset2[0])!=-1, "react.c");
		}
		else if(mptr2->tot_sunit>1 && mptr1->tot_sunit==1){
			CHECKS(syncpos(mols,mptr2,rxn_site_indx2,NULL)!=-1,"react.c");
			if(offset2[0]!=0 && offset2[1]!=0 && offset2[2]!=0)
				CHECKS(complex_pos(sim,mptr2,"pos_line2690",&offset2[0],1)!=-1,"react.c");
			CHECKS(syncpos(mols,mptr1,rxn_site_indx1,&offset1[0])!=-1, "react.c");
		}
		else{
			CHECKS(syncpos(mols,mptr1,rxn_site_indx1,&offset1[0])!=-1, "react.c");
			CHECKS(syncpos(mols,mptr2,rxn_site_indx2,&offset2[0])!=-1, "react.c");
		}

		if(sim->events) {
			if(molec_gen==1){
				fprintf(sim->events, "rxn_time=%f start ident=%s serno=%d \n", sim->time, mols->spname[mptr2->ident], mptr2->serno);
			}
			else if(molec_gen==0){
				fprintf(sim->events, "rxn_time=%f %s unbindrad=%f prob=%f dsum=%f order=%d ident1=%s ident2=%s rxn_site1=%d rxn_site2=%d mptr1->serno=%d mptr1->complex_id=%d mptr2->serno=%d mptr2->complex_id=%d\n", sim->time, rxn->rname, r, rxn->prob,dc1+dc2,order, mols->spname[mptr1->ident], mols->spname[mptr2->ident], rxn_site_indx1, rxn_site_indx2, mptr1->serno, mptr1->complex_id,mptr2->serno,mptr2->complex_id);
			}
			fprintf(sim->events,"\n");
		}	
		if(molec_gen>0 && mptr1->vchannel!=NULL){
			if(mptr1->vchannel->molec_gen>1){
				for(i=0;i<mptr1->vchannel->molec_gen;i++){
					mptr_tmp=getnextmol_cplx(mols,1,mptr2->ident);
					//if(sim->events)
					//	fprintf(sim->events,"molec_gen rxn_time=%f mptr->serno=%d\n",sim->time, mptr_tmp->serno);
					mptr_tmp->list=sim->mols->listlookup[mptr2->ident][mptr2->mstate];
					mptr_tmp->box=mptr2->box;
					mptr_tmp->mstate=mptr2->mstate;
					mptr_tmp->pnl=mptr2->pnl;
					for(d=0;d<sim->dim;d++){
						mptr_tmp->pos[d]=mptr2->pos[d];
						mptr_tmp->posx[d]=mptr2->posx[d];
						mptr_tmp->prev_pos[d]=mptr2->prev_pos[d];	
					}
					for(s=0;s<sim->mols->spsites_num[mptr2->ident];s++)
						mptr_tmp->sites[s]->value[0]=mptr2->sites[s]->value[0];
		}}}
	} 

		if(new_mol==0){
			if(order==1){ 								// a->b, a~c->b~c
				for(i=0;i<rxn->prd[0]->sites_num;i++){
					mptr1->sites[rxn->prd[0]->sites_indx[i]]->value[0]=rxn->prd[0]->sites_val[i];	
					// don't have to update the timing information for binding site if the two rct stays bound
					if(rxn->prd[0]->sites_indx[i]!=rxn->prd[0]->site_bind)
					 	mptr1->sites[rxn->prd[0]->sites_indx[i]]->time=sim->time;
					if(mptr1->sites[rxn->prd[0]->sites_indx[i]]->site_type==0)	// nonbinding site
						posptr_assign(sim->mols,mptr1,NULL,rxn->prd[0]->sites_indx[i]);
				}
				if(sim->events){ 
					fprintf(sim->events, "rxn_time=%f %s order=%d ident1=%s mptr1->serno=%d %s\n", sim->time, rxn->rname, order, mols->spname[mptr1->ident], mptr1->serno, mols->spname[mptr1->ident]);
					//fprintf(sim->events, ">>> %s serno=%d pos[0]=%f pos[1]=%f pos[2]=%f\n", rxn->rname, mptr1->serno, mptr1->pos[0], mptr1->pos[1], mptr1->pos[2]);
					//for(k=0;k<sim->mols->spsites_num[mptr1->ident];k++)
					//	if(mptr1->sites[k]->bind) fprintf(sim->events,"bound: %s:%d[%d]~%s:%d ", sim->mols->spname[mptr1->ident],mptr1->serno,k,sim->mols->spname[mptr1->sites[k]->bind->ident],mptr1->sites[k]->bind->serno);
					//fprintf(sim->events, "\n");
				}
				if(rxn->molec_num==2){
					for(i=0;i<rxn->prd[1]->sites_num;i++){
						mptr2->sites[rxn->prd[1]->sites_indx[i]]->value[0]=rxn->prd[1]->sites_val[i];	
						if(rxn->prd[1]->sites_indx[i]!=rxn->prd[1]->site_bind)
							mptr2->sites[rxn->prd[1]->sites_indx[i]]->time=sim->time;
					}
					if(sim->events){ 
						fprintf(sim->events, "%s dsum=%f order=%d ident2=%s mptr2->serno=%d %s\n", rxn->rname, dc1+dc2, order, mols->spname[mptr2->ident], mptr2->serno, mols->spname[mptr2->ident]);
				}}	
		}}
		if(new_mol==-1){						
			if(order==1 && rxn->molec_num==2){					// a~c->a
				mptr1->mstate=ms=rxn->prd[0]->prdstate;
				for(i=0;i<rxn->prd[0]->sites_num;i++){	
					site1=mptr1->sites[rxn->prd[0]->sites_indx[i]];						
					site1->value[0]=rxn->prd[0]->sites_val[i];
					site1->time=sim->time;
					if(site1->bind)
						site1->bind=NULL;
				}
				molkill(sim,mptr2,ll2,m2);
			}
			
			if(order==2){					// a+b->c 	//no need to kill mptr2, but mptr2 and mptr1 possess the same physical location	
				mptr1->mstate=ms=rxn->prd[0]->prdstate;
				mptr2->mstate=ms=rxn->prd[1]->prdstate;

				for(i=0;i<rxn->prd[0]->sites_num;i++) {
					site1=mptr1->sites[rxn->prd[0]->sites_indx[i]];
					site1->value[0]=rxn->prd[0]->sites_val[i];							// site1->value is an int*
					site1->time=sim->time;
					if(site1->site_type==2) {
						site1->bind=mptr2;
				}}
				for(i=0;i<rxn->prd[1]->sites_num;i++) {
					site2=mptr2->sites[rxn->prd[1]->sites_indx[i]];
					site2->value[0]=rxn->prd[1]->sites_val[i];
					site2->time=sim->time;
					if(site1->site_type==2) {
						site2->bind=mptr1;
					}
				}
								
				if(rxn_site_indx1>=0 && rxn_site_indx2>=0){
					for(d=0;d<dim;d++) {
						mptr1->prev_pos[d]=mptr1->pos[d];
						mptr2->prev_pos[d]=mptr2->pos[d];
						mptr1->pos[d]=mptr2->pos[d]=rxnpos[d];
						offset1[d]=mptr1->pos[d]-mptr1->prev_pos[d];
						offset2[d]=mptr2->pos[d]-mptr2->prev_pos[d];
					}
					// update complex connection		
					if(mptr1->tot_sunit>1 && mptr2->tot_sunit>1){	
						CHECKS(complex_pos(sim,mptr1,"pos_line2755",&offset1[0],1)!=-1,"react.c");
						CHECKS(complex_pos(sim,mptr2,"pos_line2756",&offset2[0],1)!=-1,"react.c");

						posptr_assign(sim->mols,mptr1,mptr2,rxn_site_indx1);
						if(rxn->rct[0]->ident!=rxn->rct[1]->ident)
							posptr_assign(sim->mols,mptr2,mptr1,rxn_site_indx2);	

						CHECKS(syncpos(mols,mptr1,rxn_site_indx1,NULL)!=-1,"react.c");
						CHECKS(syncpos(mols,mptr2,rxn_site_indx2,NULL)!=-1,"react.c");
						if(mols->complex_connect==NULL) mols->complex_connect=g_hash_table_new(g_keypair_hash,g_keypair_equal);
						entry1_ptr=GINT_TO_POINTER(g_pairing(mptr1->complex_id,mptr2->complex_id));
						entry2_ptr=GINT_TO_POINTER(g_pairing(mptr2->complex_id,mptr1->complex_id));
						g_hash_table_insert(mols->complex_connect,entry1_ptr,GINT_TO_POINTER(mptr1->s_index+1));		// so that the value won't be zero; if not, difficult to tell from NULL
						g_hash_table_insert(mols->complex_connect,entry2_ptr,GINT_TO_POINTER(mptr2->s_index+1));
					}
					else if(mptr1->tot_sunit>1 && mptr2->tot_sunit==1){
						posptr_assign(sim->mols,mptr1,mptr2,rxn_site_indx1);
						if(rxn->rct[0]->ident!=rxn->rct[1]->ident)
							posptr_assign(sim->mols,mptr2,mptr1,rxn_site_indx2);	

						CHECKS(syncpos(mols,mptr1,rxn_site_indx1,NULL)!=-1,"react.c");
						if(offset1[0]!=0 && offset1[1]!=0 && offset1[2]!=0)
							CHECKS(complex_pos(sim,mptr1,"pos_line2763",&offset1[0],1)!=-1,"react.c");
						CHECKS(syncpos(mols,mptr2,rxn_site_indx2,&offset2[0])!=-1, "react.c");
					}
					else if(mptr2->tot_sunit>1 && mptr1->tot_sunit==1){
						posptr_assign(sim->mols,mptr1,mptr2,rxn_site_indx1);
						if(rxn->rct[0]->ident!=rxn->rct[1]->ident)
							posptr_assign(sim->mols,mptr2,mptr1,rxn_site_indx2);	

						CHECKS(syncpos(mols,mptr2,rxn_site_indx2,NULL)!=-1,"react.c");
						if(offset2[0]!=0 && offset2[1]!=0 && offset2[2]!=0)
							CHECKS(complex_pos(sim,mptr2,"pos_line2768",&offset2[0],1)!=-1,"react.c");
						CHECKS(syncpos(mols,mptr1,rxn_site_indx1,&offset1[0])!=-1, "react.c");
					}
					else{
						posptr_assign(sim->mols,mptr1,mptr2,rxn_site_indx1);
						if(rxn->rct[0]->ident!=rxn->rct[1]->ident)  // for case such as dimerization
							posptr_assign(sim->mols,mptr2,mptr1,rxn_site_indx2);	
						CHECKS(syncpos(mols,mptr1,rxn_site_indx1,&offset1[0])!=-1, "react.c");
						CHECKS(syncpos(mols,mptr2,rxn_site_indx2,&offset2[0])!=-1, "react.c");
					}
					mptr1->sites[rxn_site_indx1]->bind=mptr2;
					mptr2->sites[rxn_site_indx2]->bind=mptr1;
				}
				else{
					for(d=0;d<dim;d++) {
						mptr1->prev_pos[d]=mptr1->pos[d];
						mptr2->prev_pos[d]=mptr2->pos[d];
					}
					CHECKS(complex_pos(sim,mptr1,"pos_line2755",&offset1[0],1)!=-1,"react.c");
					CHECKS(complex_pos(sim,mptr2,"pos_line2756",&offset2[0],1)!=-1,"react.c");					
				}
			 	/*	
				if(mptr1->ident==mptr2->ident){
					mptr1->from=mptr2;
					mptr2->to=mptr1;
				}
				*/
				if(sim->events) {
					difmolec(sim,mptr1,&dif_molec1);
					difmolec(sim,mptr2,&dif_molec2);
					fprintf(sim->events,"rxn_time=%f %s sqrt_bindrad2=%f dsum=%f order=%d ident1=%s ident2=%s rxn_site1=%d rxn_site2=%d mptr1->serno=%d mptr2->serno=%d mptr1->complex_id=%d mptr2->complex_id=%d mptr1->s_index=%d mptr2->s_index=%d \n", sim->time, rxn->rname, sqrt(r), dc1+dc2, order, mols->spname[mptr1->ident], mols->spname[mptr2->ident], rxn_site_indx1, rxn_site_indx2, mptr1->serno, mptr2->serno, mptr1->complex_id, mptr2->complex_id, mptr1->s_index, mptr2->s_index); 
				}
		}}
		mptr1->sites_val=molecsites_state(sim->mols,mptr1);
		if(mptr1->complex_id!=-1){
			for(mptr_tmp=mptr1->to,s=0;s<mptr1->tot_sunit-1;s++,mptr_tmp=mptr_tmp->to)
				mptr_tmp->sites_val=molecsites_state(sim->mols,mptr_tmp);
		}
		mptr1->sim_time=sim->time;
		if(mptr2){
			mptr2->sites_val=molecsites_state(sim->mols,mptr2);
			mptr2->sim_time=sim->time;
			if(mptr2->complex_id!=-1){
				for(mptr_tmp=mptr2->to,s=0;s<mptr2->tot_sunit-1;s++,mptr_tmp=mptr_tmp->to)
					mptr_tmp->sites_val=molecsites_state(sim->mols,mptr_tmp);
			}
		}
		if(sim->events) {
			//fprintf(sim->events, ">>> mptr1->serno=%d mptr1->sites_val=%d %d bindsites_val=%d\n", mptr1->serno, mptr1->sites_val, mptr1->pos==mptr1->pos_tmp, mptr1->sites[1]->bind->sites_val);
			//if(mptr2){
			//	fprintf(sim->events, ">>> mptr2->serno=%d mptr2->sites_val=%d\n", mptr2->serno, mptr2->sites_val);
			//}
			//fflush(sim->events);
		}
		return 0; 
	failure: 
	printf("rxn_time=%f, %s, mptr1->serno=%d, mptr2->serno=%d, mptr1->complex_id=%d, mptr2->complex_id=%d\n", sim->time, rxn->rname, mptr1->serno, mptr2->serno, mptr1->complex_id, mptr2->complex_id);
	printf("mptr1: pos0=%f, pos1=%f, pos2=%f, prevpos0=%f, prevpos1=%f, prevpos2=%f\n", mptr1->pos[0], mptr1->pos[1], mptr1->pos[2], mptr1->prev_pos[0], mptr1->prev_pos[1], mptr1->prev_pos[2]);
	printf("mptr2: pos0=%f, pos1=%f, pos2=%f, prevpos0=%f, prevpos1=%f, prevpos2=%f\n", mptr2->pos[0], mptr2->pos[1], mptr2->pos[2], mptr2->prev_pos[0], mptr2->prev_pos[1], mptr2->prev_pos[2]);
	printf("rxnpos[0]=%f, rxnpos[1]=%f, rxnpos[2]=%f\n", rxnpos[0], rxnpos[1], rxnpos[2]);
	return -1;

}

/* zeroreact */
int zeroreact(simptr sim) {
	int i,r,nmol;
	rxnptr rxn;
	rxnssptr rxnss;
	double pos[DIMMAX];
	panelptr pnl;
	double dc1, dc2;

	pnl=NULL;
	rxnss=sim->rxnss[0];
	if(!rxnss) return 0;
	for(r=0;r<rxnss->totrxn;r++) {
		rxn=rxnss->rxn[r];
				
		{
			nmol=poisrandD(rxn->prob);
			for(i=0;i<nmol;i++) {
				if(rxn->cmpt) compartrandpos(sim,pos,rxn->cmpt);
				else if(rxn->srf) pnl=surfrandpos(rxn->srf,pos,sim->dim);
				else systemrandpos(sim,pos);
				// if(doreact(sim,rxn,NULL,NULL,-1,-1,-1,-1,pos,pnl,NULL,NULL,NULL,dc1,dc2)) return 1;
			}
			sim->eventcount[ETrxn0]+=nmol; }}
	return 0; }


/* unireact */
int unireact(simptr sim) {
	rxnssptr rxnss;
	rxnptr rxn;
	moleculeptr *mlist,mptr;
	moleculeptr mptr_tmp, mptr1, mptr2;
	int *nrxn,**table; 
	int i,j,k,s,m,nmol,ll,entry;
	enum MolecState ms;
	int cond_flag1, cond_flag2;
	GSList *r;
	intptr_t r_indx,tmp_len;
	int **Mlist;
	//Mlist=sim->mols->Mlist;
	// cal0 h gate is always 1
	double v, inf_n, tau_n, rate_open, rate_close; //inf_h, tau_h, vhalf_n, slope_n, vhalf_h, slope_h;
	double prob_OC, prob_OB, prob_OO, block_f, block_r;
	int prd_indx, sites_val,len, list_len,molec_gen;
	double dc1,dc2,rnd_prob,rxn_prob0,prob_acc,ica,n_t;
	
	if(!sim->rxnss[1]) return 0;
	for(ll=0;ll<sim->mols->nlist;ll++){
		mlist=sim->mols->live[ll];
		nmol=sim->mols->nl[ll];
		for(m=0;m<nmol;m++) {
			mptr1=mlist[m];
			//mptr1=mlist[Mlist[ll][m]];
			//if(mptr1->sim_time==sim->time) continue;
			entry=g_pairing(mptr1->ident,mptr1->sites_val);
			ms=mptr1->mstate;

			rxnss=sim->rxnss[1];
			if(!rxnss) continue;
			r=(GSList*)g_hash_table_lookup(rxnss->table,GINT_TO_POINTER(entry));
			tmp_len=(intptr_t)g_hash_table_lookup(rxnss->entrylist,r);
			list_len=(int)tmp_len;

			for(len=0,prob_acc=1;r,len<list_len;len++,r=r->next){
				r_indx=(intptr_t)r->data;				
				rxn=rxnss->rxn[(int)r_indx];
				
				// generate varying number of Ca2+ ions
				if(sim->mols->volt_dependent[mptr1->ident]==1){
					if(mptr1->vchannel->vtime==sim->time){
						// if gate is open	
						molec_gen=mptr1->vchannel->molec_gen=-1;
						v=mptr1->vchannel->voltage;
						block_f=61.2 ; // 0.036 mM^-1 ms^-1= 0.036 uM^-1 s^-1= 0.036 *1700
						block_r=0.018;

						// inf_n=1/(1+exp((v+15.0)/(-4.98)));
						inf_n=1/(1+exp((v+15.0)/(-8.0)));
						// tau_n=0.06+0.75*4*sqrt(0.32*0.68)/(exp((v+15.0)*0.68/4.98) + exp(-(v+15.0)*0.32/4.98));
						tau_n=0.06+0.75*4*sqrt(0.45*0.55)/(exp((v+15.0)*0.55/8.0) + exp(-(v+15.0)*0.45/8.0));
						rate_open=double(inf_n/tau_n);
						rate_close=double((1-inf_n)/tau_n);
						n_t=inf_n*(1-exp(-sim->time/tau_n));
						if(mptr1->sites[0]->value[0]==1){
							// inf_h=1/(1+exp((v-1000)/10));
							// tau_h=200*4*0.5/(exp(-(v-1000)*0.5/10) + exp((v-1000)*0.5/10)); 
						
							prob_OC=1.0-exp(-rate_close*sim->dt);
							prob_OO=1-prob_OC;

							if(rxn->prd[0]->ident==mptr1->ident) prd_indx=0;
							else prd_indx=1;
							if(rxn->prd[prd_indx]->sites_val[0]==0){
								rxn->prob=prob_OC;
							}
							else if(rxn->prd[prd_indx]->sites_val[0]==1){	
								rxn->prob=prob_OO;		
							}
						}
					
						//ica=2.5*(v-34)*exp(-(v-34.0)/12.0)/(1-exp(-(v-34.0)/12.0));	
						ica=5.0*(v+1.90955)*exp(-(v+1.90955)/12.0)/(1-exp(-(v+1.90955)/12.0));	

						// to convert fA to pA by dividing 1000
						rnd_prob=randCOD();
						if(mptr1->sites[0]->value[0]==0){		// gate is closed
							rxn->prob=1.0-exp(-rate_open*sim->dt);
							if(rnd_prob>rxn->prob) 
								break;
							else{								// calculate how many ions to generate
								// single channel conductance 5.0 pS Keller et al. 2.5 pS
								// ica is fitted to ghk_i in fA/um2 using eq.S3 from Tadross et al. 2013
								// n_gate_cal0=2
								molec_gen=(int)floor(ica*3.12*sim->dt);
								if(molec_gen<=0)
									break;
								else { mptr1->vchannel->molec_gen=molec_gen;}
							}
						}
						else{  // gate is open
							if(rnd_prob>=rxn->prob && list_len>1){	
								r=r->next;
								r_indx=(intptr_t)r->data;
								rxn=rxnss->rxn[(int)r_indx];
							}
							if(rxn->nprod==2){
								molec_gen=(int)floor(ica*3.12*sim->dt);
								if(molec_gen<=0)
									break;	
								else{ mptr1->vchannel->molec_gen=molec_gen;}
							}
						}	

						printf("unireact time=%f %s prob=%f rnd_prob=%f n_t=%f v=%f molec_gen=%d serno=%d list_len=%d pos[2]=%f\n", sim->time, rxn->rname, rxn->prob, rnd_prob, n_t,v, molec_gen,mptr1->serno, list_len, mptr1->pos[2]);
						if(doreact(rxn->rxnss,r,mptr1,NULL,ll,m,-1,-1,NULL,NULL,NULL,NULL,NULL,dc1,dc2)){
							printf("line 2908, unireact, doreact() failed, %s\n", rxn->rname);
							return 1;
						}	
						else break;
				}}
				else if(mptr1->vchannel==NULL){
					if(randCOD()>=rxn->prob*prob_acc){
						prob_acc*=(1-rxn->prob);
						continue;
					}
					if(!rxn->permit[ms])	 continue;											// failed permit test
					if(rxn->cmpt) { if(!posincompart(sim,mptr1->pos,rxn->cmpt))	continue;}			// failed compartment test
					if(rxn->srf) { if(!mptr1->pnl || mptr1->pnl->srf!=rxn->srf)	continue;}			// failed surface test
					if(doreact(rxn->rxnss,r,mptr1,NULL,ll,m,-1,-1,NULL,NULL,NULL,NULL,NULL,dc1,dc2)){
						printf("line 2922, unireact, doreact() failed, %s\n", rxn->rname);
						return 1;
					}	
					else break;
				}	

	}}}

	return 0; }


/* morebireact */
int morebireact(rxnssptr rxnss,gpointer rptr,moleculeptr mptrA,moleculeptr mptrB,int ll1,int m1,int ll2,enum EventType et,double *vect, int rxn_site_indx1,int rxn_site_indx2,double radius,double dc1,double dc2) {
	// moleculeptr mptrA,mptrB;
	int d,swap;
	enum MolecState ms,msA,msB;
	moleculeptr mptr_tmp;
	double offset[DIMMAX]; // pos_tmp;
	int cplx_connect,i1,i2;
	gpointer entry_ptr, entryr_ptr;
	simptr sim;
	rxnptr rxn;
	moleculeptr dif_molec1,dif_molec2;

	sim=rxnss->sim;
	rxn=rxnss->rxn[(int)(intptr_t)((GSList*)rptr)->data];
	
	if(rxn->cmpt && !(posincompart(sim,mptrA->pos,rxn->cmpt) && posincompart(sim,mptrB->pos,rxn->cmpt))) return 0;
	if(rxn->srf && !((mptrA->pnl && mptrA->pnl->srf==rxn->srf) || (mptrB->pnl && mptrB->pnl->srf==rxn->srf))) return 0;

	msA=mptrA->mstate;
	msB=mptrB->mstate;
	if(msA==MSsoln && msB!=MSsoln)
		msA=(panelside(mptrA->pos,mptrB->pnl,sim->dim,NULL,0)==PFfront)?MSsoln:MSbsoln;
	else if(msB==MSsoln && msA!=MSsoln)
		msB=(panelside(mptrB->pos,mptrA->pnl,sim->dim,NULL,0)==PFfront)?MSsoln:MSbsoln;
	ms=(MolecState)(msA*MSMAX1+msB);
	
	// check if the two molecules belong two complexes that are already bound
	// cplx_connect=complex_connect(sim,mptr1,mptr2);
	if(sim->mols->complex_connect){
		entry_ptr=GINT_TO_POINTER(g_pairing(mptrA->complex_id,mptrB->complex_id));
		entryr_ptr=GINT_TO_POINTER(g_pairing(mptrB->complex_id,mptrA->complex_id));
		if(g_hash_table_lookup(sim->mols->complex_connect,entry_ptr) || g_hash_table_lookup(sim->mols->complex_connect,entryr_ptr))	
			cplx_connect=1;
	}
	else cplx_connect=0;
	if(cplx_connect>0){
		printf("react.c line 2786, time=%f\n", sim->time);
		return 0;
	}

	if(rxn->permit[ms]) {
		if(et==ETrxn2wrap && rxn->rparamt!=RPconfspread) {			// if wrapping, then move faster diffusing molecule
			i1=difmolec(sim,mptrA,&dif_molec1);
			i2=difmolec(sim,mptrB,&dif_molec2);
				
			if(sim->mols->difc[i1][dif_molec1->mstate]<sim->mols->difc[i2][dif_molec2->mstate]){
				for(d=0;d<sim->dim;d++) {
					mptrB->prev_pos[d]=mptrB->pos[d];  				// cplx
					mptrB->posoffset[d]-=(mptrA->pos[d]+vect[d])-mptrB->pos[d];
					mptrB->pos[d]=mptrA->pos[d]+vect[d]; 
					offset[d]=mptrB->pos[d]-mptrB->prev_pos[d];		// cplx
				}
				if(complex_pos(sim,mptrB,"pos_line3059",&offset[0],1)==-1){
					printf("react.c line 3060, time=%f, %s, mptrA->serno=%d, mptrB->serno=%d\n", sim->time, rxn->rname, mptrA->serno, mptrB->serno);
					return -1;
				}
			}	
			else{
				for(d=0;d<sim->dim;d++) {
					mptrA->prev_pos[d]=mptrA->pos[d];		
					mptrA->posoffset[d]-=(mptrB->pos[d]-vect[d])-mptrA->pos[d];
					mptrA->pos[d]=mptrB->pos[d]-vect[d]; 
					offset[d]=mptrA->pos[d]-mptrA->prev_pos[d];
				}
				if(complex_pos(sim,mptrA,"pos_line3071",&offset[0],1)==-1){
					printf("react.c line 3072, time=%f, %s, mptrA->serno=%d, mptrB->serno=%d\n", sim->time, rxn->rname, mptrA->serno, mptrB->serno);
					return -1;
			}}}	

		sim->eventcount[et]++;
		return doreact(rxn->rxnss,rptr,mptrA,mptrB,ll1,m1,ll2,-1,NULL,NULL,rxn_site_indx1,rxn_site_indx2,radius,dc1,dc2);
	}
	return 0; }



/* bireact */
int bireact(simptr sim,int neigh) {
	int dim,maxspecies,ll,ll1,ll2,i,j,s,d,*nl,nmol2,b2,m1,m2,bmax,wpcode,maxlist;
	int *nrxn; // ,**table;
	double dist2,vect[DIMMAX];
	rxnssptr rxnss;
	rxnptr rxn,*rxnlist;
	boxptr bptr;
	moleculeptr **live,*mlist2,mptr1,mptr2,mptrA,mptrB;
	int bind_site_indx1, bind_site_indx2, rxn_site_indx1,rxn_site_indx2;
	int doreact_flag, list_len,len;
	GSList *r, *r_tmp;
	intptr_t r_indx;
	siteptr site_tmp;
	int **Mlist;
	double dsum,bindrad2,unbindrad;
	double rnd, dc1, dc2, acc_prob;
	gpointer adjusted;

	rxnss=sim->rxnss[2];
	if(!rxnss) return 0;
	dim=sim->dim;
	live=sim->mols->live;
	maxspecies=rxnss->maxspecies;
	maxlist=rxnss->maxlist;
	nrxn=rxnss->nrxn;
	rxnlist=rxnss->rxn;
	nl=sim->mols->nl;
	Mlist=sim->mols->Mlist;

	if(!neigh) {																		// same box
		for(ll1=0;ll1< sim->mols->nlist;ll1++)
			for(m1=0;m1<nl[ll1];m1++) {
				//mptr1=live[ll1][Mlist[ll1][m1]];
				mptr1=live[ll1][m1];
				if(sim->multibinding==0){
					if(mptr1->sim_time==sim->time) continue; }
				bptr=mptr1->box;
				for(ll2=ll1;ll2<sim->mols->nlist;ll2++){
					mlist2=bptr->mol[ll2];
					nmol2=bptr->nmol[ll2];
					for(m2=0;m2<nmol2;m2++) {
						mptr2=mlist2[m2];
						if(mptr2->serno<=mptr1->serno) continue;
						//printf("sim->multibinding=%d\n",sim->multibinding);
						if(sim->multibinding==0){
							if(mptr1->sim_time==sim->time || mptr2->sim_time==sim->time) continue;}
						if(rxnss->binding[mptr1->ident][mptr2->ident]==0) continue;

						if(mptr1->pos==mptr2->pos ){
							r=bireact_test(sim,1,mptr1,mptr2,&list_len,&dc1,&dc2,NULL);
							// mistakes would occur for same reactants identity reactions					
							if(!r) { 
								r=bireact_test(sim,1,mptr2,mptr1,&list_len,NULL,NULL,NULL);
							}
							if(!r) continue;

							for(len=0,r_tmp=r,acc_prob=1;len<1;len++,r_tmp=r_tmp->next){
								rxn=rxnss->rxn[(int)(intptr_t)r_tmp->data];
								if(mptr1->ident==rxn->rct[0]->ident ) {	mptrA=mptr1;mptrB=mptr2;  }
								else { mptrA=mptr2;mptrB=mptr1;  }
								/*	
								rnd=randCOD();
								if(rnd>=rxn->prob) {
									acc_prob*=(1-rxn->prob);
									//continue;
									break;
								}
								*/
								for(s=0;s<rxn->rct[0]->sites_num;s++){
									if(mptrA->sites[rxn->rct[0]->sites_indx[s]]->time==sim->time) goto site_loop0;}
								for(s=0;s<rxn->rct[1]->sites_num;s++){
									if(mptrB->sites[rxn->rct[1]->sites_indx[s]]->time==sim->time) goto site_loop0;}

								if(mptrA->sim_time==sim->time) 
									printf("sim->time=%f rname:%s  mptrA->serno=%d\n",sim->time,rxn->rname,mptrA->serno);
								if(rxn->cmpt) { if(!posincompart(sim,mptrA->pos,rxn->cmpt))	break;}			// failed compartment test
								if(rxn->srf) { if(!mptrA->pnl || mptrA->pnl->srf!=rxn->srf)	break;}			// failed surface test
								doreact_flag=doreact(rxn->rxnss,r_tmp,mptrA,mptrB,ll1,m1,ll2,m2,NULL,NULL,rxn->prd[0]->site_bind,rxn->prd[1]->site_bind,NULL,dc1,dc2);
								if(doreact_flag==-1) return 1;		
						 		else if(doreact_flag!=0 && doreact_flag!=-1) return 1;	
						 		//else {break;}
							}
						}
						else{

							r=bireact_test(sim,2,mptr1,mptr2,&list_len,&dc1,&dc2,&bindrad2);						
							if(!r) r=bireact_test(sim,2,mptr2,mptr1,&list_len,&dc2,&dc1,&bindrad2);					
							if(!r) continue;

							rxn=rxnss->rxn[(int)(intptr_t)r->data];
							if(mptr1->ident==rxn->rct[0]->ident) {	mptrA=mptr1;mptrB=mptr2;  }
							else { mptrA=mptr2;mptrB=mptr1;  }
							
							for(s=0;s<rxn->rct[0]->sites_num;s++){
								if(mptrA->sites[rxn->rct[0]->sites_indx[s]]->time==sim->time){ 
									//printf("%s %f mptrA->serno=%d s=%d\n",rxn->rname,sim->time,mptrA->serno,s);
									goto site_loop0;
								}
							}
							for(s=0;s<rxn->rct[1]->sites_num;s++){
								if(mptrB->sites[rxn->rct[1]->sites_indx[s]]->time==sim->time) 
									goto site_loop0;
							}
								
							//site_tmp=mptrA->sites[rxn->prd[0]->site_bind];
							//if(site_tmp->time==sim->time) goto site_loop0;
							rxn_site_indx1=rxn->prd[0]->site_bind;

							//site_tmp=mptrB->sites[rxn->prd[1]->site_bind];
							//if(site_tmp->time==sim->time) goto site_loop0;
							rxn_site_indx2=rxn->prd[1]->site_bind;
							
							if(mptrA->sim_time==sim->time) 
								printf("sim->time=%f rname:%s  mptrA->serno=%d\n",sim->time,rxn->rname,mptrA->serno);
							//if(sim->events && mptrA->ident==2 && mptrB->ident==1)
							//	fprintf(sim->events,"time=%f mptrA->serno=%d %d line3237\n", sim->time, mptrA->serno, mptrA->pos==mptrA->pos_tmp);

							if((rxn->prob==1 || randCOD()<rxn->prob) && (mptrA->mstate!=MSsoln || mptrB->mstate!=MSsoln || !rxnXsurface(sim,mptrA,mptrB,rxn_site_indx1,rxn_site_indx2))) {
								if(morebireact(rxn->rxnss,r,mptrA,mptrB,ll1,m1,ll2,ETrxn2intra,NULL,rxn_site_indx1,rxn_site_indx2,bindrad2,dc1,dc2)){ 
									if(sim->events){ 
										fprintf(sim->events,"time=%f mptr1->pos==mptr2->pos: %d %s\n", sim->time,mptr1->pos==mptr2->pos, rxn->rname);
										return 2;	
									}
								}	
								//else {break;}
							}
						}
						site_loop0:continue;
		}}}}
	else {																			// neighbor box, must be for binding reactions
		for(ll1=0;ll1< sim->mols->nlist;ll1++)
			for(m1=0;m1<nl[ll1];m1++) {
				//mptr1=live[ll1][Mlist[ll1][m1]];
				mptr1=live[ll1][m1];
				if(sim->multibinding==0){
					if(mptr1->sim_time==sim->time) continue; }
				bptr=mptr1->box;
				for(ll2=ll1;ll2<sim->mols->nlist;ll2++){
					bmax=(ll1!=ll2)?bptr->nneigh:bptr->midneigh;
					for(b2=0;b2<bmax;b2++) {
						mlist2=bptr->neigh[b2]->mol[ll2];
						nmol2=bptr->neigh[b2]->nmol[ll2];
						if(bptr->wpneigh && bptr->wpneigh[b2]) {					  // neighbor box with wrapping
							wpcode=bptr->wpneigh[b2];
							for(m2=0;m2<nmol2;m2++) {
								mptr2=mlist2[m2];
								if(mptr2->serno <= mptr1->serno) continue;
								if(sim->multibinding==0){
									if(mptr1->sim_time==sim->time || mptr2->sim_time==sim->time) continue;}

								if(rxnss->binding[mptr1->ident][mptr2->ident]==0) continue;
								r=bireact_test(sim,2,mptr1,mptr2,&list_len,&dc1,&dc2,&bindrad2);						
								if(!r) r=bireact_test(sim,2,mptr2,mptr1,&list_len,&dc2,&dc1,&bindrad2);					
								if(!r) continue;

								rxn=rxnss->rxn[(int)(intptr_t)r->data];
								if(mptr1->ident==rxn->rct[0]->ident){ mptrA=mptr1;mptrB=mptr2;	}
								else { mptrA=mptr2;mptrB=mptr1;}

								for(s=0;s<rxn->rct[0]->sites_num;s++){
									if(mptrA->sites[rxn->rct[0]->sites_indx[s]]->time==sim->time){ 
										//printf("%s %f mptrA->serno=%d s=%d\n",rxn->rname,sim->time,mptrA->serno,s);
										goto site_loop1;
								}}
								for(s=0;s<rxn->rct[1]->sites_num;s++){
									if(mptrB->sites[rxn->rct[1]->sites_indx[s]]->time==sim->time) goto site_loop1;}
								
								//site_tmp=mptrA->sites[rxn->prd[0]->site_bind];
								//if(site_tmp->time==sim->time) goto site_loop0;
								rxn_site_indx1=rxn->prd[0]->site_bind;

								//site_tmp=mptrB->sites[rxn->prd[1]->site_bind];
								//if(site_tmp->time==sim->time) goto site_loop0;
								rxn_site_indx2=rxn->prd[1]->site_bind;
								if(sim->events && mptrA->ident==2 && mptrB->ident==1)
									fprintf(sim->events,"time=%f mptrA->serno=%d %d line3297\n", sim->time, mptrA->serno, mptrA->pos==mptrA->pos_tmp);
	
								if(mptrA->sim_time==sim->time) 
									printf("sim->time=%f rname:%s mptrA->serno=%d\n", sim->time, rxn->rname, mptrA->serno);
								if((rxn->prob==1 || randCOD()<rxn->prob) && mptrA->ident!=0 && mptrB->ident!=0) {
									if(morebireact(rxn->rxnss,r,mptrA,mptrB,ll1,m1,ll2,ETrxn2wrap,vect,rxn_site_indx1,rxn_site_indx2,bindrad2,dc1,dc2)){ 
										printf("react.c line 2946, rxn name: %s\n", rxn->rname);
										return 3;
									}
									//else {break; }
								}
								site_loop1:continue;
							}}
							else													// neighbor box, no wrapping
								for(m2=0;m2<nmol2;m2++) {
									mptr2=mlist2[m2];
									if(mptr2->serno <= mptr1->serno) continue;
									if(sim->multibinding==0){
										if(mptr1->sim_time==sim->time || mptr2->sim_time==sim->time) continue;}
									if(rxnss->binding[mptr1->ident][mptr2->ident]==0) continue;
									r=bireact_test(sim,2,mptr1,mptr2,&list_len,&dc1,&dc2,&bindrad2);						
									if(!r) r=bireact_test(sim,2,mptr2,mptr1,&list_len,&dc2,&dc1,&bindrad2);				
									if(!r) continue;

									rxn=rxnss->rxn[(int)(intptr_t)r->data];
									if(mptr1->ident==rxn->rct[0]->ident) { mptrA=mptr1;mptrB=mptr2;	}
									else { mptrA=mptr2;mptrB=mptr1;}
									
									for(s=0;s<rxn->rct[0]->sites_num;s++){
										if(mptrA->sites[rxn->rct[0]->sites_indx[s]]->time==sim->time){
											//printf("%s %f mptrA->serno=%d s=%d\n",rxn->rname,sim->time,mptrA->serno,s);
											goto site_loop2;
									}}
									for(s=0;s<rxn->rct[1]->sites_num;s++){
										if(mptrB->sites[rxn->rct[1]->sites_indx[s]]->time==sim->time) goto site_loop2;}
									
									//site_tmp=mptrA->sites[rxn->prd[0]->site_bind];
									//if(site_tmp->time==sim->time) goto site_loop0;
									rxn_site_indx1=rxn->prd[0]->site_bind;

									//site_tmp=mptrB->sites[rxn->prd[1]->site_bind];
									//if(site_tmp->time==sim->time) goto site_loop0;
									rxn_site_indx2=rxn->prd[1]->site_bind;
									if(sim->events && mptrA->ident==2 && mptrB->ident==1)
										fprintf(sim->events,"time=%f mptrA->serno=%d %d line3341\n", sim->time, mptrA->serno, mptrA->pos==mptrA->pos_tmp);

									if(mptrA->sim_time==sim->time) 
										printf("sim->time=%f rname:%s mptrA->serno=%d\n", sim->time, rxn->rname, mptrA->serno);
									if((rxn->prob==1||randCOD()<rxn->prob) && (mptrA->mstate!=MSsoln || mptrB->mstate!=MSsoln || !rxnXsurface(sim,mptrA,mptrB,rxn_site_indx1,rxn_site_indx2)) && mptrA->ident!=0 && mptrB->ident!=0) {
										if(morebireact(rxn->rxnss,r,mptrA,mptrB,ll1,m1,ll2,ETrxn2inter,NULL,rxn_site_indx1,rxn_site_indx2,bindrad2,dc1,dc2)){
											printf("react.c line 2972, rxn name: %s\n", rxn->rname);
											return 4;
										}
										//else { break; }
									}
									site_loop2:continue;
								}
						}}}}

	return 0; }

void posptr_assign(molssptr mols, moleculeptr mptr, moleculeptr mptr_bind, int site){
	// dif_molec determines the diffusion coefficient but not necessarily determine a molecules' physical location (pos_tmp, pos)
	int sitecode,s;
	GSList *r;
	intptr_t r_indx;
	moleculeptr mptr_tmp, bind_difmolec;
	complexptr cplx_tmp;

	if(mptr->complex_id>-1){
		cplx_tmp=mols->complexlist[mptr->complex_id];
		// cam_bK rxn, bK needs to keep its dif_molec in cav
		// in case cplx_tmp->dif_bind is a different subunit from mptr
		// no need to reassign diffuse molecule for complex subunits if already exist
		// in case cav_bK_r
		// if(cplx_tmp->dif_molec && cplx_tmp->dif_bind) return;
	}
	else cplx_tmp=NULL;
	
	if(mptr->pos[0]!=mptr->pos_tmp[0] || mptr->pos[1]!=mptr->pos_tmp[1] || mptr->pos[2]!=mptr->pos_tmp[2]){
		mptr->pos_tmp[0]=mptr->pos[0];
		mptr->pos_tmp[1]=mptr->pos[1];
		mptr->pos_tmp[2]=mptr->pos[2];
	}

	if(mols->spdifsites){
		r=(GSList*)g_hash_table_lookup(mols->spdifsites,GINT_TO_POINTER(mptr->ident));
		if(!r){ 
			mptr->dif_molec=NULL; 
			mptr->pos=mptr->pos_tmp;
			mptr->dif_site=-1;
			return;
		}
		while(r){
			r_indx=(intptr_t)r->data;
			sitecode=(int)r_indx;
		
			if(mptr->sites[sitecode]->value[0]==1){ 
				//if(mptr->sites[sitecode]->site_type==1){
					// consider bK-camN2C2 (where bK itself attached to cav)
					// in case such as NgcamN1~ca_r, !mptr_bind means its unbinding rxn; and case NgcamN1~ca
					if(mptr->sites[sitecode]->bind){
						difmolec(mols->sim,mptr->sites[sitecode]->bind,&bind_difmolec);
						mptr->dif_molec=bind_difmolec;
						mptr->dif_site=sitecode;
						mptr->bind_id=bind_difmolec->bind_id;
						goto site_cplx; 
					}

					if(mptr_bind && site==sitecode){							// for binding reactions
						difmolec(mols->sim,mptr_bind,&bind_difmolec);
						if(mptr->complex_id==-1){
							mptr->dif_molec=bind_difmolec;
							mptr->dif_site=sitecode;
							// mptr->pos=mptr->dif_molec->pos;					// may not be true, cam binds to a non dominant bK subunit
							mptr->pos=mptr_bind->pos;
							mptr->bind_id=bind_difmolec->bind_id;
						}
						else{
							cplx_tmp=mols->complexlist[mptr->complex_id];
							// if(cplx_tmp->dif_molec) return;
							cplx_tmp->dif_molec=mptr->dif_molec=bind_difmolec;
							cplx_tmp->dif_bind=mptr;
							cplx_tmp->dif_bind_site=sitecode;
							// mptr->pos=mptr->dif_molec->pos;					// can't link a subunit pos to its dif_molec pos
							mptr->dif_site=sitecode;
							mptr->pos=mptr_bind->pos;
						}
						goto site_cplx; 	
					}
				//}
			}
			else if(mptr->sites[sitecode]->bind==mptr && mptr->bind_id>0) // consider the case bK@A bind to Actin
				mptr->bind_id=-1;
			r=r->next;
		}
		// consider cav_bK_r, bKp-camN2C2 (where bK next attached to cav), Ng-camN0C0_r
		if(!cplx_tmp) { mptr->dif_molec=NULL; mptr->dif_site=-1;mptr->pos=mptr->pos_tmp; mptr->bind_id=mptr->ident;return;} 
		else if (cplx_tmp->dif_bind==mptr){
			mptr->dif_molec=NULL;
			mptr->dif_site=-1;			
			mptr->pos=mptr->pos_tmp;
			mptr->bind_id=mptr->ident;
			goto site_cplx;
		}
	} else{ mptr->dif_molec=NULL; mptr->dif_site=-1;mptr->pos=mptr->pos_tmp; }

	// also for every monomers attached to the marcromolecules, e.g., cam attached to camkii
	site_cplx:if(cplx_tmp){
			if(cplx_tmp->dif_bind==mptr){	
				cplx_tmp->dif_molec=mptr->dif_molec;
				if(mptr->dif_molec) 
					cplx_tmp->dif_bind_site=sitecode;
				else{
					cplx_tmp->dif_bind_site=-1; cplx_tmp->dif_bind=cplx_tmp->dif_molec=cplx_tmp->zeroindx_molec;}
			}
		for(s=1,mptr_tmp=mptr->to;s<mptr->tot_sunit;s++,mptr_tmp=mptr_tmp->to){
			mptr_tmp->dif_molec=cplx_tmp->dif_molec;
		}			
	}
	return;
}


/*reassign dif_molec and pos, consider two tasks: update pos, update dif_molec */
int syncpos(molssptr mols, moleculeptr mptr, int site, double *offset){
	int k,d,s;
	moleculeptr mptr_bind,mptr_tmp;

	for(k=0;k<mols->spsites_num[mptr->ident];k++){
		if(mptr->sites[k]->bind){
			mptr_bind=mptr->sites[k]->bind;
			if(k!=site && mptr_bind->dif_site!=-1){
				if(mptr_bind->sites[mptr_bind->dif_site]->bind==mptr){
					mptr_bind->pos=mptr->pos;
					if(mptr->dif_molec)				// in case cam binds to a bK subunit, whose dif_molec is NULL
						mptr_bind->dif_molec=mptr->dif_molec;
					syncpos(mols,mptr_bind,k,NULL);
	}}}}

	if(site==-1) return 0;
	if(mptr->complex_id>-1){
		for(s=1,mptr_tmp=mptr->to; s< mptr->tot_sunit && mptr_tmp!=mptr; s++,mptr_tmp=mptr_tmp->to){
			syncpos(mols,mptr_tmp,-1,NULL);
	}}

	if(offset){
		if(offset[0]!=0 && offset[1]!=0 && offset[2]!=0){
		for(k=0;k<mols->spsites_num[mptr->ident];k++){
			if(mptr->sites[k]->bind && k!=site){		// k!=site, in case of "cav_bK", mptr1=bK, mptr2=cav
				mptr_bind=mptr->sites[k]->bind;	
				if(k!=site && mptr_bind->complex_id>-1 && site!=-1){
					if(complex_pos(mols->sim,mptr_bind,"pos_line3353",offset,1)==-1){
						printf("line 3453 mptr->serno=%d mptr_bind->serno=%d\n",mptr->serno,mptr_bind->serno);
						return -1;
						}
					}
		}}
		/*
		if(mptr->to!=NULL){
			for(d=0;d<mols->sim->dim;d++){ 
				mptr->to->prev_pos[d]=mptr->to->pos[d];
				mptr->to->pos[d]=mptr->pos[d];
		}}
		*/
	}}

	return 0;
}


double rxnparser(molssptr mols, char *str, int *molec_num, int *orderptr,int *nprodptr, rct_mptr *rct1ptr, rct_mptr *rct2ptr, prd_mptr *prd1ptr, prd_mptr *prd2ptr){
	char s0[STRCHAR], *rhs, *lhs, *molec1, *molec2;
	int i;
	double rate;

	strcpy(s0,str);
	rhs=strsplit(str,"->");
	lhs=str;
	// printf("rhs:%s\n", rhs);
	// printf("lhs:%s\n", lhs);

	if(*rct1ptr==NULL) *rct1ptr=(rct_mptr) malloc(sizeof(struct rctmolecule));
	if(*rct2ptr==NULL) *rct2ptr=(rct_mptr) malloc(sizeof(struct rctmolecule));
	if(*prd1ptr==NULL) *prd1ptr=(prd_mptr) malloc(sizeof(struct prdmolecule));
	if(*prd2ptr==NULL) *prd2ptr=(prd_mptr) malloc(sizeof(struct prdmolecule));
	(*rct1ptr)->states=NULL;
	(*rct2ptr)->states=NULL;
	(*prd1ptr)->sites_indx=NULL;
	(*prd2ptr)->sites_indx=NULL;
	(*prd1ptr)->sites_val=NULL;
	(*prd2ptr)->sites_val=NULL;		
	(*prd1ptr)->site_bind=-1;
	(*prd2ptr)->site_bind=-1;

	if(strchr(lhs,'~') && strchr(rhs,'+')) {				// A~B->A+B
		molec2=strsplit(lhs,"~");
		molec1=lhs;
		*molec_num=2;
		*orderptr=1;
		*nprodptr=2;
		
		if(rxnlhs(mols,molec1,*rct1ptr)!=0) goto failure;
		if(rxnlhs(mols,molec2,*rct2ptr)!=0) goto failure;
		molec2=strsplit(rhs,"+");
		molec1=rhs;
		rate=rxnrhs(mols,molec1,molec2,*prd1ptr,*prd2ptr);
	}
	else if(strchr(rhs,'~') && strchr(lhs,'+')) {			// A+B->A~B
		molec2=strsplit(lhs,"+");
		molec1=lhs;
		*molec_num=2;
		*orderptr=2;
		*nprodptr=1;
		
		if(rxnlhs(mols,molec1,*rct1ptr)!=0) goto failure;
		if(rxnlhs(mols,molec2,*rct2ptr)!=0) goto failure;
		molec2=strsplit(rhs,"~");
		molec1=rhs;
		rate=rxnrhs(mols,molec1,molec2,*prd1ptr,*prd2ptr);
	}
	else if(strchr(lhs,'~') && strchr(rhs,'~')) {			// A~B->A'~B' or A~B->A'~B
		molec2=strsplit(lhs,"~");
		molec1=lhs;
		*molec_num=2;
		*orderptr=1;
		*nprodptr=1;
		// printf("line 3024, molec1:%s, molec2:%s\n", molec1, molec2);
		if(rxnlhs(mols,molec1,*rct1ptr)!=0) goto failure;
		if(rxnlhs(mols,molec2,*rct2ptr)!=0) goto failure;
		molec2=strsplit(rhs,"~");
		// printf("line 3019, molec2:%s, rhs:%s\n", molec2, rhs);
		rate=rxnrhs(mols,rhs,molec2,*prd1ptr,*prd2ptr);
	}
	else if(strchr(lhs,'~') && !strchr(rhs,'~') && !strchr(rhs,'+')) {	// pump~ca -> pump
		molec2=strsplit(lhs,"~");
		molec1=lhs;	
		*molec_num=2;
		*orderptr=1;
		*nprodptr=0;		// to differentiate from previous cases, means molecules are not conserved
		if(rxnlhs(mols,molec1,*rct1ptr)!=0) goto failure;
		if(rxnlhs(mols,molec2,*rct2ptr)!=0) goto failure;
		rate=rxnrhs(mols,molec1,NULL,*prd1ptr,*prd2ptr);
	}
	else if(!strchr(lhs,'+') && !strchr(lhs,'~') && strchr(rhs,'+')) { // CaL->CaL + ca 
		molec2=strsplit(rhs,"+");
		molec1=rhs;
		printf("line 3283, molec1:%s, molec2: %s\n", molec1, molec2);
		*molec_num=1;	
		*orderptr=1;
		*nprodptr=2;
		if(rxnlhs(mols,lhs,*rct1ptr)!=0) goto failure;
		rate=rxnrhs(mols,molec1,molec2,*prd1ptr,*prd2ptr);
		free(*rct2ptr);*rct2ptr=NULL;	
	}
	else if(strchr(lhs,'+') && strchr(rhs,'+')) { // bK + gK -> bK + gK 
		molec2=strsplit(lhs,"+");
		molec1=lhs;
		printf("line 3283, molec1:%s, molec2: %s\n", molec1, molec2);
		*molec_num=2;	
		*orderptr=2;
		*nprodptr=1;  // to do with the new_mol variable in doreact()
		if(rxnlhs(mols,molec1,*rct1ptr)!=0) goto failure;
		if(rxnlhs(mols,molec2,*rct2ptr)!=0) goto failure;
		//rate=-1;
		molec2=strsplit(rhs,"+");
		molec1=rhs;
		rate=rxnrhs(mols,molec1,molec2,*prd1ptr,*prd2ptr);
	}
	else {													
		*molec_num=1;
		*orderptr=1;
		*nprodptr=1;
		
		if(rxnlhs(mols,lhs,*rct1ptr)!=0) goto failure;
		rate=rxnrhs(mols,rhs,NULL,*prd1ptr,*prd2ptr);
		free(*rct2ptr); *rct2ptr=NULL;
		free(*prd2ptr);	*prd2ptr=NULL;
	}
	return rate;
	failure: return NULL;
}

char *strtrim(char *str){
	char *end;
	
	// trim leading space
	while(isspace(*str)) str++;
	if(*str==0)	return str;					// all spaces;
	// trim trailing space
	end = str+strlen(str)-1;
	while(end>str && isspace(*end)) end--;

	// write new null terminator
	*(end+1)=0;

	return str;
}


/* the inputs are all char *, not &(char *) or char ** as in strsep, easier to debug in gdb 
	will change the original string
*/
char* strsplit(char *str0, char* delim){
	char *part;
	int i;
	
	if(!(part=strstr(str0,delim)))	return NULL;
	*part='\0';
	part+=strlen(delim);		// strlen("->")=2;	
	return part;
}

/* extract the contents from a pair of brackets, assuming not nested 
	will change the original string
*/
char* strextract(char *str0, char* symbols){
	if(strlen(symbols)!=2) return NULL;			// strlen("{}")=2;
	char *content,*rest;

	content=strchr(str0, symbols[0]);	
	rest=strrchr(str0, symbols[1]);

	if(!content) return NULL;
	if(!rest) return NULL;

	*content='\0';
	content++; 

	*rest='\0';
	rest++; 
	
	return content;
}

/* read in rxncond in {}, msstate in ()*/
int rxnlhs(molssptr mols, char *molec, rct_mptr rct){
	char *ms;	
	char *cond;

	ms=strextract(molec,"()");
	cond=strextract(molec,"{}");
	printf("molec:%s, cond:%s\n",molec,cond);

	if(ms) rct->rctstate=molstring2ms(ms);
	else rct->rctstate=MSsoln;
	rct->ident=stringfind(mols->spname,mols->nspecies,strtrim(molec));
	if(rct->ident==-1) {printf("wrong rct site name\n"); return -1;}
	if(cond){
		rct->states_num=rxncond_parse(mols,cond,rct->ident,&(rct->states),&(rct->sites_num),&(rct->sites_indx));	
		if(rct->states_num<0)
			return -1;
	}
	rct->difc=NULL;
	return 0; 
}

double rxnrhs(molssptr mols, char *molec1, char *molec2, prd_mptr prd1, prd_mptr prd2) {
	char *sites1, *sites2, *site1_val, *site2_val, *ms1, *ms2;
	double tail;
	char s0[STRCHAR],*str;
	int k;
	
	if(molec2){ 
		molec2=strtrim(molec2);
		strcpy(s0,molec2);
		for(k=0;k<strlen(s0) && !isspace(s0[k]);k++);	
		// printf("k=%d\n",k);
		molec2[k]='\0';
		while(isspace(s0[k]))	k++;
		str=s0+k;
		// printf("str:%s, molec2:%s\n",str,molec2);
		sscanf(str,"%lf", &tail);
	}
	else{
		molec1=strtrim(molec1);
		strcpy(s0,molec1); 
		for(k=0;k<strlen(s0) && !isspace(s0[k]);k++);
		molec1[k]='\0';
		while(isspace(s0[k])) k++;	
		str=s0+k;	
		sscanf(str,"%lf", &tail);
	}
	molec1=strtrim(molec1);
	ms1=strextract(molec1,"()");
	sites1=strextract(molec1,"[]");
	// printf("rxnrhs, molec1:%s\n", molec1);
	prdsites_parse(mols,sites1,molec1,prd1);
	if(ms1)	prd1->prdstate=molstring2ms(ms1);
	else prd1->prdstate=MSsoln;

	if(molec2) {
		molec2=strtrim(molec2);
		ms2=strextract(molec2,"()");
		// printf("rxnrhs, molec2:%s\n", molec2);
		sites2=strextract(molec2,"[]");
		prdsites_parse(mols,sites2,molec2,prd2);
		if(ms2) prd2->prdstate=molstring2ms(ms2);
		else prd2->prdstate=MSsoln;
	}
	return tail;
}

void prdsites_parse(molssptr mols, char *sites_state_str, char *molec_name, prd_mptr prd){
	int k, sites_num, prd_ident, *sites_indx, *sites_val;
	char *cptr, *swap, *str_tmp, *sitename_str, *siteval_str;

	prd_ident=stringfind(mols->spname,mols->nspecies,molec_name);
	if(prd_ident==-1) {printf("wrong site name\n"); return;}
	prd->ident=prd_ident;
	if(sites_state_str==NULL) {
		prd->sites_indx=NULL;
		prd->sites_val=NULL;
		prd->sites_num=0;
		prd->site_bind=-1;
		return;	
	}

	sites_num=0;
	cptr=sites_state_str;
	// printf("prdsites_parse, cptr:%s\n",cptr);
	while(cptr=strchr(cptr,',')){
		cptr++;
		sites_num++;
	}
	sites_indx=(int*) calloc(sites_num+1,sizeof(int));
	sites_val=(int*) calloc(sites_num+1,sizeof(int));
	for(k=0;k<sites_num+1;k++){
		str_tmp=strsplit(sites_state_str,",");		
		swap=str_tmp;
		str_tmp=sites_state_str;
		sites_state_str=swap;
		
		sitename_str=str_tmp;
		siteval_str=strsplit(sitename_str,"=");
		sites_indx[k]=stringfind(mols->spsites_name[prd_ident],mols->spsites_num[prd_ident],sitename_str);
		if(prd->site_bind==-1 && mols->spsites_binding[prd_ident][sites_indx[k]]==1){
			prd->site_bind=sites_indx[k];
		}
		// sprintf(siteval_str,"%i", sites_val[k]);
		sscanf(siteval_str,"%i",&sites_val[k]);
	}
	
	prd->sites_indx=sites_indx;
	prd->sites_val=sites_val;
	prd->sites_num=sites_num+1;
	return;
}

/*return states_num as int */
int rxncond_parse(molssptr mols, char *cond, int rct_ident, int **sites_state_ptr, int *sites_num, int **sites_indx){
	char *condstr,*cond_tmp, *cond_val, *swap, *cptr;
	int i, k, k1, s, cond_num, sitecode, site_indx, rct_ident1; 
	int *sitecode_tmp, bi_sitesval, sitecode_count, sites_state_num, sites_state_tmp;
	char **rxncond;	

	cond_num=0;
	cptr=cond;
	while(cptr=strchr(cptr,',')){
		cptr++;	
		cond_num++;
	}
	sitecode_tmp=(int*) calloc(cond_num+1,sizeof(int));
	sites_num[0]=cond_num+1;

	sitecode_count=0;
	bi_sitesval=0;
	condstr=EmptyString();
	for(i=0;i<cond_num+1;i++){
		cond_tmp=strsplit(cond,",");
		swap=cond_tmp;
		cond_tmp=cond;
		cond=swap;
		sitecode_tmp[i]=-1;

		if(!cond_tmp) cond_tmp=cond;
		strcpy(condstr,cond_tmp);		

		// printf("cond:%s, swap:%s, cond_tmp:%s, condstr:%s\n", cond, swap, cond_tmp, condstr);
		if((cond_val=strsplit(condstr,"=="))!=NULL){
			// printf("cond_val:%s\n",cond_val);
			sscanf(cond_val,"%i",&sites_state_tmp);
			// printf("i=%d, cond_val:%s, condstr:%s, sites_state_tmp=%d\n", i, cond_val, condstr, sites_state_tmp);
			condstr=strtrim(condstr);	
			sitecode=stringfind(mols->spsites_name[rct_ident], mols->spsites_num[rct_ident], condstr);
			sitecode_tmp[i]=sitecode;
			if(sitecode>=0) {
				sitecode_count++;
				bi_sitesval+=power(2,sitecode_tmp[i])*sites_state_tmp; // [i];
			}
			else{
				printf("sitecode error: %s\n", condstr);
				return -1;
			}
		}
	}
	// if(sitecode_count>mols->spsites_num[rct_ident]) return NULL;	
	sites_state_num=(int)power(2,mols->spsites_num[rct_ident]-sitecode_count);	// sites_state_num>=2^0=1

	if(sitecode_count>0){
		sites_state_ptr[0]=(int*) calloc(sites_state_num,sizeof(int));
		sites_state_ptr[0][0]=bi_sitesval;
		// sites_state_ptr[0][sites_state_num]=-1;
	
		k=1;
		for(s=0;s<mols->spsites_num[rct_ident];s++){
			for(i=0;i<sitecode_count && s!=sitecode_tmp[i];i++);				// the variable i was used to loop around sitecode_tmp array
			if(i>=sitecode_count && k<sites_state_num){							// so that site s is not one of the sitecode_tmp sites
				// printf("line 3269, s=%d, i=%d, sitecode_tmp[i]=%d, k=%d, sites_state_num=%d\n", s, i, sitecode_tmp[i],k, sites_state_num);
				for(k1=0;k1<k;k1++){
					sites_state_ptr[0][k+k1]=sites_state_ptr[0][k1]+(int)power(2,s);
					// printf("line 3297, k1=%d, k=%d, s=%d, sites_state_ptr[0][k+k1]=%d, sites_state_ptr[0][k1]=%d\n", k1, k, s, sites_state_ptr[0][k+k1], sites_state_ptr[0][k1]);
				}
				k=k+k1;
			}
		}}
	else {
		sites_state_ptr[0]=NULL;												// in case reactant site states are not set
		sites_state_num=0;
	}

	//free(sitecode_tmp);
	sites_indx[0]=sitecode_tmp;
	return sites_state_num;	
}


GSList* bireact_test(simptr sim, int order, moleculeptr mptr1, moleculeptr mptr2, int *len, double *dc1, double *dc2, double *bindrad2) {
	int j,k1,k2,k1x,k2x,entry,entryx,indx_r,rnd,s,l,r_indx,left_indx,right_indx;
	rxnssptr rxnss;	
	rxnptr rxn,rxnx,rxn_tmp;
	GSList *r, *rx, *r_tmp,*r_rxn,*rtmp_ptr;
	intptr_t tmp_len;
	double rnd_prob,aff_tot,dist2,prob_assign,prob_max,numerator,denominator,aff,kar,bindrad_eff,dsum,ka_tot,kar_tot,kcr_tot,p;
	gpointer probh,probl,adjusted,rev;
	double *affptr,*bindradptr,*problptr,*probhptr,prob_survive,bindrad,ka,kdiff,miu,phi;	
	int list_len,site1,site2,entry_sites,entry_rxn;

	for(s=0,site1=-1;s<sim->mols->spsites_num[mptr1->ident];s++){
		// in case as bKpcamN1C0, the binding site cam doesnt change state, but nonbinding site P changes state
		//if(mptr1->sites[s]->time==sim->time) return NULL;
		if(order==1){
			if(site1==-1 && mptr1->sites[s]->bind){
				if(mptr1->sites[s]->bind==mptr2){ 
					//if(mptr1->sites[s]->time==sim->time) return NULL;
					site1=s;
		}}}}
	for(s=0,site2=-1;s<sim->mols->spsites_num[mptr2->ident];s++){
		//if(mptr2->sites[s]->time==sim->time) return NULL;
		if(order==1){
			if(site2==-1 && mptr2->sites[s]->bind){
				if(mptr2->sites[s]->bind==mptr1) { 
					//if(mptr2->sites[s]->time==sim->time) return NULL;
					site2=s;
		}}}}

	rxnss=sim->rxnss[2];
	k1=g_pairing(mptr1->ident,mptr1->sites_val);
	k2=g_pairing(mptr2->ident,mptr2->sites_val);
	entry=g_pairing(k1,k2);

	// use only the updated states of molecules but allow only one binding/unbinding event per pair of binding site
	r=(GSList*)g_hash_table_lookup(rxnss->table,GINT_TO_POINTER(entry));
	if(r==NULL) {
		*len=0; return NULL;	}
	if(rxnss->rxn[(int)(intptr_t)r->data]->order!=order) {
		*len=0; return NULL;	}

	*len=(int)(intptr_t)g_hash_table_lookup(rxnss->entrylist,r);
	if(order==1){
		prob_assign=prob_max=0;
		
		entry_sites=g_pairing(site1,site2);	
		entry_rxn=g_pairing(entry,entry_sites);
		r_rxn=(GSList*)g_hash_table_lookup(rxnss->rxn_ord1st,GINT_TO_POINTER(entry_rxn));
		if(r_rxn==NULL){
			for(l=0,r_tmp=r;l<len[0];r_tmp=r_tmp->next,l++){
				rxn_tmp=rxnss->rxn[(int)(intptr_t)r_tmp->data];
				if(rxn_tmp->nprod==2){
					if(site1==rxn_tmp->prd[0]->site_bind && site2==rxn_tmp->prd[1]->site_bind){
						g_hash_table_insert(rxnss->rxn_ord1st,GINT_TO_POINTER(entry_rxn),g_slist_append((GSList*)g_hash_table_lookup(rxnss->rxn_ord1st,GINT_TO_POINTER(entry_rxn)),(gpointer)r_tmp));		
					}
					else continue;
				}
				else if(rxn_tmp->nprod==1){
					g_hash_table_insert(rxnss->rxn_ord1st,GINT_TO_POINTER(entry_rxn),g_slist_append((GSList*)g_hash_table_lookup(rxnss->rxn_ord1st,GINT_TO_POINTER(entry_rxn)),(gpointer)r_tmp));
			}}
			r_rxn=(GSList*)g_hash_table_lookup(rxnss->rxn_ord1st,GINT_TO_POINTER(entry_rxn));
			list_len=(int)g_slist_length(r_rxn);
		}
		else if(r_rxn==(GSList*)-1) list_len=0;
		else{
			r_rxn=(GSList*)g_hash_table_lookup(rxnss->rxn_ord1st,GINT_TO_POINTER(entry_rxn));
			list_len=(int)g_slist_length(r_rxn);
		}
		if(list_len==0){		
			r_rxn=(GSList*)-1;
			return NULL;
		}
		else if(list_len==1){
			rnd_prob=randCOD();
			r_tmp=(GSList*)r_rxn->data;
			rxn_tmp=rxnss->rxn[(int)(intptr_t)r_tmp->data];
			if(rnd_prob<rxn_tmp->prob){
				len[0]=1;
				return (GSList*)r_rxn->data;
			}
			else return NULL;
		}
		else{
			problptr=(double*)g_hash_table_lookup(rxnss->probrng_l,(GSList*)r_rxn->data);
			probhptr=(double*)g_hash_table_lookup(rxnss->probrng_h,(GSList*)r_rxn->data);
		
			if(problptr==NULL || probhptr==NULL){
				for(l=0,r_tmp=r_rxn;l<list_len;r_tmp=r_tmp->next,l++){
					rtmp_ptr=(GSList*)r_tmp->data;
					rxn_tmp=rxnss->rxn[(int)(intptr_t)rtmp_ptr->data];
					problptr=(double*)malloc(sizeof(double));
					memcpy(problptr,&prob_assign,sizeof(double));
					g_hash_table_insert(rxnss->probrng_l,(gpointer)rtmp_ptr,problptr);
					if(rxn_tmp->rate>0)
						prob_assign+=rxn_tmp->rate;
					else if(rxn_tmp->rate==-2){
						prob_assign+=-log(1.0-exp(-prob_assign*sim->dt))/sim->dt;
					}
					probhptr=(double*)malloc(sizeof(double));
					memcpy(probhptr,&prob_assign,sizeof(double));
					g_hash_table_insert(rxnss->probrng_h,(gpointer)rtmp_ptr,probhptr);
				}
				prob_max=prob_assign;
			}	
			else{
				for(l=0,r_tmp=r_rxn;l<list_len-1;r_tmp=r_tmp->next,l++);
				prob_max=((double*)g_hash_table_lookup(rxnss->probrng_h,(GSList*)r_tmp->data))[0];
			}

			prob_survive=exp(-prob_max*sim->dt);
			rnd_prob=randCOD();
			if(rnd_prob<prob_survive){
				len[0]=0;
				return NULL;
			}
			rnd_prob=randCOD();
			for(l=0,r_tmp=r_rxn;l<list_len;r_tmp=r_tmp->next,l++){	
				probl=g_hash_table_lookup(rxnss->probrng_l,(GSList*)r_tmp->data);
				probh=g_hash_table_lookup(rxnss->probrng_h,(GSList*)r_tmp->data);
				if(rnd_prob<((double*)probh)[0]/prob_max && rnd_prob>=((double*)probl)[0]/prob_max){
					len[0]=1;
					return (GSList*)r_tmp->data;
				}
			}
	}}
	else if(order==2){		
		dist2=molec_distance(sim,mptr1->pos,mptr2->pos);
		if(len[0]==1){
			bindrad2[0]=radius(sim,r,mptr1,mptr2,dc1,dc2,NULL);		
			if(dist2<bindrad2[0]){
				return r;
			}
			else{
				len[0]=0;		
				return NULL;
			}
		}		
		else if(len[0]>1){
			bindradptr=(double*)g_hash_table_lookup(rxnss->bindrad_eff,r);
			if(!bindradptr){
				aff_tot=0;
				prob_assign=0;
				numerator=0;
				denominator=0;
				prob_max=0;

				dsum=MolCalcDifcSum(sim,mptr1,mptr2,dc1,dc2);
				for(l=0,r_tmp=r;l<len[0];r_tmp=r_tmp->next,l++){
					rxn_tmp=rxnss->rxn[(int)(intptr_t)r_tmp->data];
					printf("%s\n",rxn_tmp->rname);
					//rev=findreverserxn(rxnss,r_tmp,mptr1,mptr2);
					//if(!rev) continue;
					if(rxn_tmp->order!=2) continue;

					//kna=actrxnrate(sqrt(2*sim->dt*dsum),bindrad)/sim->dt;
					//ka=1.0/(1.0/kna+1.0/(4*PI*bindrad*dsum));
					ka=rxn_tmp->rate;
					//Oct 6, don't think need to find reverse rxn at this stage
					//kar=rxnss->rxn[(int)(intptr_t)((GSList*)rev)->data]->rate;
					/*
					if(g_hash_table_lookup(rxnss->rxnaff,r_tmp)==NULL){
						aff=ka/kar;
						affptr=(double*)malloc(sizeof(double));
						memcpy(affptr,&aff,sizeof(double));			
						g_hash_table_insert(rxnss->rxnaff,(gpointer)r_tmp,affptr);
 					}
					else{
						affptr=(double*)g_hash_table_lookup(rxnss->rxnaff,r_tmp);	
						aff=((double*)affptr)[0];
					}
					*/		
					ka_tot+=ka;	
					//kar_tot+=kar;
			
				//kcr_tot=0;	
				//for(l=0,r_tmp=r;l<len[0];r_tmp=r_tmp->next,l++)
				//	rxn_tmp=rxnss->rxn[(int)(intptr_t)r_tmp->data];
				//	rev=findreverserxn(rxnss,r_tmp,mptr1,mptr2);
				//	if(!rev) continue;
				//	if(rxn_tmp->order!=2) continue;
				//	ka=rxn_tmp->rate;
				//	kar=rxnss->rxn[(int)(intptr_t)((GSList*)rev)->data]->rate;

					problptr=(double*)g_hash_table_lookup(rxnss->probrng_l,r_tmp);
					probhptr=(double*)g_hash_table_lookup(rxnss->probrng_h,r_tmp);
					if(problptr==NULL || probhptr==NULL){				
						problptr=(double*)malloc(sizeof(double));
						memcpy(problptr,&prob_assign,sizeof(double));
						g_hash_table_insert(rxnss->probrng_l,(gpointer)r_tmp,problptr);
						//p=ka/kar/ka_tot;	
						//kcr_tot+=p*kar;
						//kinetics_ratio(sim,dsum,ka,NULL,&p);	
						//prob_assign+=ka*(1+1.0/p);
						prob_assign+=ka;
						//printf("%s bindrad=%f\n",rxn_tmp->rname,bindrad);
						probhptr=(double*)malloc(sizeof(double));
						memcpy(probhptr,&prob_assign,sizeof(double));
						g_hash_table_insert(rxnss->probrng_h,(gpointer)r_tmp,probhptr);
					}
				}

				//ka_tot*=kcr_tot;
				bindrad_eff=bindingradius(ka_tot,sim->dt,dsum,0,0);
				if(unbindingradius(0.2,sim->dt,dsum,bindrad_eff)>0)
					bindrad_eff=bindingradius(ka_tot*0.8,sim->dt,dsum,-1,0);
				//kinetics_ratio(sim,dsum,ka_tot,&bindrad_eff,NULL);
				printf("ka_tot=%f prob_assign=%f rc3=%f\n",ka_tot, prob_assign, bindrad_eff);
				bindradptr=(double*)malloc(sizeof(double));
				memcpy(bindradptr,&bindrad_eff,sizeof(double));
				g_hash_table_insert(rxnss->bindrad_eff,(gpointer)r_tmp,bindradptr);
				prob_max=prob_assign;
			}
			else{ 
				bindradptr=(double*)g_hash_table_lookup(rxnss->bindrad_eff,r); 
				for(l=0,r_tmp=r;l<len[0]-1;r_tmp=r_tmp->next,l++);
				//bindrad2[0]=radius(sim,r_tmp,mptr1,mptr2,dc1,dc2,NULL);
				//printf("r->data=%d bindradptr[0]=%f,r_tmp->data=%d, l=%d\n",(int)(intptr_t)r->data,bindradptr[0],(int)(intptr_t)r_tmp->data,l);
				prob_max=((double*)g_hash_table_lookup(rxnss->probrng_h,r_tmp))[0];
			}
			bindrad_eff=bindradptr[0];
				
			if(dist2<bindrad_eff*bindrad_eff);
			else {len[0]=0;return NULL;}
			
			//if(dist2<32870); // 167
			//else {len[0]=0; return NULL; }
			rnd_prob=randCOD();

			for(l=0,r_tmp=r;l<len[0];r_tmp=r_tmp->next,l++){
				probl=g_hash_table_lookup(rxnss->probrng_l,r_tmp);
				probh=g_hash_table_lookup(rxnss->probrng_h,r_tmp);
				if(rnd_prob<((double*)probh)[0]/prob_max && rnd_prob>=((double*)probl)[0]/prob_max){
					bindrad2[0]=radius(sim,r_tmp,mptr1,mptr2,dc1,dc2,NULL);
					return r_tmp;
				}	
			}
			/*
			for(l=0,r_tmp=r;l<len[0];r_tmp=r_tmp->next,l++){
				rxn_tmp=rxnss->rxn[(int)(intptr_t)r_tmp->data];
				if(g_hash_table_lookup(rxnss->adjusted_kon,r_tmp)==NULL){
					rev=findreverserxn(rxnss,r_tmp,mptr1,mptr2);
					adjk_tmp=aff_max*rxnss->rxn[(int)(intptr_t)((GSList*)rev)->data]->rate;
				
					adjk_ptr=(double*)malloc(sizeof(double));
					memcpy(adjk_ptr,&adjk_tmp,sizeof(double));	
					g_hash_table_insert(rxnss->adjusted_kon,(gpointer)r_tmp,adjk_ptr);
				}	
				else{
					adjusted=g_hash_table_lookup(rxnss->adjusted_kon,r_tmp);
					adjk_tmp=((double*)adjusted)[0];
				}}
			*/
		}}

	return r;
}

int molecsites_state(molssptr mols, moleculeptr mptr){
	int s, state_s;
	int sites_state=0;
	complexptr cplx_tmp;
	/*
	if(mptr->ident==2 && mptr->sites[5]->value[0]==1)
		printf("react.c 3418  cam.Kp==1 \n");
	if(mptr->ident==3 && mptr->sites[2]->value[0]==1 && mptr->sites[0]->value[0]==1)
		printf("react.c 3420 bK cam==1 & n.cam==1 \n");
	if(mptr->ident==4 && mptr->sites[2]->value[0]==1 && mptr->sites[0]->value[0]==1)
		printf("react.c 3422 gK cam==1 & n.cam==1 \n");
	*/	
	
	if(mptr->complex_id!=-1)
		cplx_tmp=mols->complexlist[mptr->complex_id];
	else cplx_tmp=NULL;
	for(s=0;s< mols->spsites_num[mptr->ident];s++){
		state_s=mptr->sites[s]->value[0];
		/*
		// not practical for the case bK@A-Actin when bK is not the zero index subunit, and if bK->sites[4]->value[0]==0
		// if not, the reaction bK-cam_A_r will repeatedly executed
		if(cplx_tmp){
			if(cplx_tmp->dif_molec){
				if(cplx_tmp->dif_bind_site==s)
					state_s=cplx_tmp->dif_bind->sites[s]->value[0];				
		}}
		*/
		sites_state+=(int)power(2,s)*state_s;
	
	}
	return sites_state;
}

guint g_pairing (guint A, guint B){
	guint h;
	h=A>=B? A*A+A+B:A+B*B;
	return h;
}

guint g_keypair_hash (gconstpointer v){
	guint A,B,h;
	A=((gint *)v)[0];
	B=((gint *)v)[1];
	h=g_pairing(A,B);
	return h;
}

int g_keypair_equal(gconstpointer v1,gconstpointer v2){
	int A1,B1,A2,B2;

	A1=((gint *)v1)[0];
	B1=((gint *)v1)[1];
	A2=((gint *)v2)[0];
	B2=((gint *)v2)[1];
	if(A1==A2 && B1==B2) return 1;
	return 0;
}

int gpointer_cmpr(gconstpointer v1,gconstpointer v2){
	return *(int*)v1-*(int*)v2;
}

/*
int r_pair_equal(r_pair p1, r_pair p2){
	if(p1.first==p2.first && p1.second==p1.second) return 1;	
	else return 0;
}
*/


double radius(simptr sim, gpointer rptr, moleculeptr mptr1, moleculeptr mptr2, double *dc1, double *dc2, int molec_gen){
	double difadj1,difadj2, r, rate3, bindrad2, unbindrad, dsum;
	double *radius_ptr, *radiusrev_ptr;
	char adj1[30], adj2[30];
	//moleculeptr dif_molec1, dif_molec2;
	rxnptr rxn;
	gpointer rev,r_tmp;
	double pg;
	
	dsum=MolCalcDifcSum(sim,mptr1,mptr2,dc1,dc2);

	if(molec_gen!=NULL){
		if(molec_gen==1) return 0;	}
	
	r_tmp=g_hash_table_lookup(sim->rxnss[2]->radius,rptr);
	if(r_tmp)
		r=((double*)r_tmp)[0];
	if(!r_tmp){
		rxn=sim->rxnss[2]->rxn[(int)(intptr_t)((GSList*)rptr)->data];
		if(rxn->rate!=-1)
			rev=findreverserxn(rxn->rxnss,rptr,mptr1,mptr2);
		else 
			rev=NULL;
		if(rev==NULL && rxn->order==1){
			rxn->prdpos[0][0]=0;
			rxn->prdpos[0][1]=0;	
			return 0;
		}

		if(rxn->order==2){
			rate3=rxn->rate;
		}
		if(rxn->order==1) rate3=rxn->rxnss->rxn[(int)(intptr_t)((GSList*)rev)->data]->rate;
		if(mptr1->ident==mptr2->ident && rxn->rate!=-1) rate3*=2;
	
		// smoldyn-2.43/source/Smoldyn/smolreact.c line 1247
		if(rxn->order==2 && rev==NULL){
			if(rate3==-1 && rxn->bindrad>0){
				r=rxn->bindrad*rxn->bindrad;
			}
			else{
				bindrad2=bindingradius(rate3,sim->dt,dsum,-1,0);
				rxn->bindrad=bindrad2;
				bindrad2*=bindrad2;
				r=bindrad2;
			}
		}
		if(rev){
			pg=rxn->rparam;
			// smoldyn-2.43/.../smolreact.c lin 732
			
			bindrad2=bindingradius(rate3,sim->dt,dsum,0,0);			
			unbindrad=unbindingradius(pg,sim->dt,dsum,bindrad2);
			
			if(unbindrad>0.0){
				bindrad2=bindingradius(rate3*(1-pg),sim->dt,dsum,-1,0);
				unbindrad=unbindingradius(pg,sim->dt,dsum,bindrad2);
			}
			else 
				unbindrad=0;

			rxn->bindrad=bindrad2;
			sim->rxnss[2]->rxn[(int)(intptr_t)((GSList*)rev)->data]->bindrad=bindrad2;
			rxn->unbindrad=unbindrad;
			sim->rxnss[2]->rxn[(int)(intptr_t)((GSList*)rev)->data]->unbindrad=unbindrad;

			bindrad2*=bindrad2;
			if(rxn->order==2 && rxn->nprod==1){
				radiusrev_ptr=(double*)malloc(sizeof(double));
				memcpy(radiusrev_ptr,&unbindrad,sizeof(double));
				g_hash_table_insert(rxn->rxnss->radius,rev,radiusrev_ptr);
				radius_ptr=(double*)malloc(sizeof(double));
				memcpy(radius_ptr,&bindrad2,sizeof(double));
				g_hash_table_insert(rxn->rxnss->radius,rptr,radius_ptr);
				r=bindrad2;	
			}
			else if(rxn->order==1 && rxn->nprod==2){
				// smoldyn-2.43/source/Smoldyn/smolreact.c line 1401
				radiusrev_ptr=(double*)malloc(sizeof(double));
				memcpy(radiusrev_ptr,&bindrad2,sizeof(double));
				g_hash_table_insert(rxn->rxnss->radius,rev,radiusrev_ptr);
				radius_ptr=(double*)malloc(sizeof(double));
				memcpy(radius_ptr,&unbindrad,sizeof(double));
				g_hash_table_insert(rxn->rxnss->radius,rptr,radius_ptr);
				r=unbindrad;
			}	
		}
	}
	return r;
}


int difmolec(simptr sim,moleculeptr mptr,moleculeptr *dif_molecptr){
	moleculeptr dif_molec, cplx_dif;
	int ident;

	cplx_dif=NULL;
	if(mptr->complex_id==-1){
		if(mptr->dif_molec){
			dif_molec=mptr->dif_molec;
			if(dif_molec->complex_id!=-1){
				cplx_dif=sim->mols->complexlist[dif_molec->complex_id]->dif_molec;
				ident=cplx_dif->bind_id; 
			}
			else { ident=dif_molec->bind_id;}
		}
		else{
			dif_molec=mptr;
			ident=mptr->ident;
		}
	}
	else{	
		cplx_dif=sim->mols->complexlist[mptr->complex_id]->dif_molec;
		if(cplx_dif){
			dif_molec=cplx_dif;	
		}
		else{
			dif_molec=mptr->dif_molec?mptr->dif_molec:mptr;
		}
		ident=dif_molec->bind_id;
	}

	dif_molecptr[0]=dif_molec;
	return ident;
}		
