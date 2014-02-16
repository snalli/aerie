#include "bcs/main/common/rtconfig.h"
#include "scm/scm/model.h"
#include "common/errno.h"
#include <iostream>

int STAMNOS_SCM_LATENCY_WRITE = 0;
int ScmModel::RuntimeConfig::scm_latency_write = 0;

int
ScmModel::RuntimeConfig::Init()
{
	int  ret;
	int  val;

	if ((ret = Config::Lookup("scmmodel.latency", &val)) < 0) {
		return ret;
	}
	scm_latency_write = val;
	
	return E_SUCCESS;
}

int 
ScmModel::Init()
{
	RuntimeConfig::Init();

	STAMNOS_SCM_LATENCY_WRITE = ScmModel::RuntimeConfig::scm_latency_write;

	return E_SUCCESS;
}
