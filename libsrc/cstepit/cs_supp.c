
#include "quip_config.h"

/*
 * linkage to C stepit
 */

#include <math.h>

#include "quip_prot.h"
//#include "fitsine.h"
#include "cstepit.h"
#include "optimize.h"

static int n_prms;

/* local variables */
// BUG not thread-safe
static float (*stept_user_func)(void);

static void init_cstepit_params(SINGLE_QSP_ARG_DECL)
{
	double xmin[MAX_OPT_PARAMS];
	double xmax[MAX_OPT_PARAMS];
	double deltx[MAX_OPT_PARAMS];
	double delmn[MAX_OPT_PARAMS];
	double ans[MAX_OPT_PARAMS];
	List *lp;
	Node *np;
	Opt_Param *opp;
	int i,n;
	int nfmax;		/* max. # function calls */

	lp = opt_param_list(SGL_DEFAULT_QSP_ARG);
	if( lp == NO_LIST ) return;

	n_prms=eltcount(lp);
	n=reset_n_params(n_prms);
	if( n != n_prms ) n_prms = n;

	np=lp->l_head;
	i=0;
	while( np!= NO_NODE && i < n_prms ){
		opp = (Opt_Param *)(np->n_data);

		xmin[i]=opp->minv;
		xmax[i]=opp->maxv;
		deltx[i]=opp->delta;
		delmn[i]=opp->mindel;
		ans[i]=opp->ans;

		i++;
		np=np->n_next;
	}
	nfmax=100000;

	/* copy to fortran */

	setvals(QSP_ARG  ans,n_prms);
	setminmax(QSP_ARG  xmin,xmax,n_prms);
	setdelta(QSP_ARG  deltx,delmn,n_prms);
//advise("SETTING ntrace to 1 FOR MAX DEBUG!");
//	settrace(1);
	setmaxcalls(nfmax);
}

static void cstepit_scr_funk(void)
{
	char str[128];
	float	err;
	Variable *vp;
	int i;
	List *lp;
	Node *np;
	double ans[MAX_OPT_PARAMS];

	/* ooh, icky:  getvals fetches global vars from cstepit module... */

	getvals(ans,n_prms);

	if( opt_func_string==NULL ){
		NWARN("No optimization string defined");
		return;
	}

	lp=opt_param_list(SGL_DEFAULT_QSP_ARG);
	if( lp == NO_LIST ){
		NWARN("No optimization parameters to vary!?");
		err=0.0;
		setfobj((double)err);
		return;
	}
	np=lp->l_head;

	/* stepit has passed us params in the ans array -
	 * we want to get them into named variables...
	 */
	i=0;
	while(np!=NO_NODE && i < n_prms ){
		Opt_Param *opp;

		opp = (Opt_Param *)( np->n_data);
		sprintf(str,"%g",ans[i]);	/* why add 1?  fortan? */
		assign_var(DEFAULT_QSP_ARG  opp->op_name,str);
		i++;
		np=np->n_next;
	}

	/* We used to call pushtext here, but we like digest
	 * because it automatically pushes and pops the top menu.
	 *
	 * chew_text doesn't work, however, because it doesn't block
	 * the interpreter, which returns to the terminal...
	 *
	 * We have a problem - calling optimization from another callback
	 * function causes it to exit when done!?
	 * It turns out that that was because older scripts (written
	 * for the old version that didn't push the top menu automatically)
	 * didn't have a quit after the call to optimize - ???
	 */

	digest(DEFAULT_QSP_ARG  opt_func_string, OPTIMIZER_FILENAME);
	
	vp=var__of(DEFAULT_QSP_ARG  "error");
	if( vp == NO_VARIABLE ) {
		NWARN(DEFAULT_ERROR_STRING);
		sprintf(DEFAULT_ERROR_STRING,
	"variable \"error\" not set by script fragment \"%s\"!?",
			opt_func_string);
		err=0.0;
	} else sscanf(VAR_VALUE(vp),"%g",&err);

	setfobj((double)err);
}

COMMAND_FUNC( run_cstepit_scr )
{
	init_cstepit_params(SINGLE_QSP_ARG);
	stepit(QSP_ARG  cstepit_scr_funk);
}

static void evaluate_error_c(void)
{
	double	err;
	double	x[MAX_OPT_PARAMS];
	int i;
	List *lp;
	Node *np;

	getvals(x,n_prms);		/* get the parameter estimates */

	lp=opt_param_list(SGL_DEFAULT_QSP_ARG);
	np=lp->l_head;
	i=0;
	while(np!=NO_NODE && i < n_prms ){
		Opt_Param *opp;

		opp = (Opt_Param *)(np->n_data);
		opp->ans = (float) x[i];
		i++;
		np=np->n_next;
	}

	err=(*stept_user_func)();

	setfobj(err);
}

void run_cstepit_c(QSP_ARG_DECL  float (*func)())
{
	init_cstepit_params(SINGLE_QSP_ARG);

	stept_user_func = func;

	stepit(QSP_ARG  evaluate_error_c);
}

