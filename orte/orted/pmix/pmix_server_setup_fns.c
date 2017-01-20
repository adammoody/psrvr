/*
 * Copyright (c) 2004-2010 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2011 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart,
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 * Copyright (c) 2006-2013 Los Alamos National Security, LLC.
 *                         All rights reserved.
 * Copyright (c) 2009      Cisco Systems, Inc.  All rights reserved.
 * Copyright (c) 2011      Oak Ridge National Labs.  All rights reserved.
 * Copyright (c) 2013-2017 Intel, Inc.  All rights reserved.
 * Copyright (c) 2014      Mellanox Technologies, Inc.
 *                         All rights reserved.
 * Copyright (c) 2014-2016 Research Organization for Information Science
 *                         and Technology (RIST). All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 *
 */

#include "orte_config.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <fcntl.h>

#include "opal_stdint.h"
#include "opal/types.h"
#include "opal/util/argv.h"
#include "opal/util/output.h"
#include "opal/util/error.h"
#include "opal/mca/hwloc/base/base.h"
#include "opal/mca/pmix/pmix.h"

#include "orte/util/name_fns.h"
#include "orte/runtime/orte_globals.h"
#include "orte/runtime/orte_wait.h"
#include "orte/mca/errmgr/errmgr.h"
#include "orte/mca/rmaps/rmaps_types.h"

#include "pmix_server_internal.h"
#include "pmix_server.h"

static void process(int sd, short args, void *cbdata)
{

}

static void infocbfunc(int status,
                       opal_list_t *info,
                       void *cbdata,
                       opal_pmix_release_cbfunc_t release_fn,
                       void *release_cbdata)
{
    /* this is in the PMIx progress thread, so we need
     * to thread-shift it into our own */

}
/* stuff proc attributes for sending back to a proc */
int orte_pmix_server_setup_job(orte_job_t *jdata,
                               opal_buffer_t *buffer)
{
    int rc;
    orte_proc_t *pptr;
    int i, k, n;
    opal_list_t *info, *pmap;
    opal_value_t *kv;
    orte_node_t *node, *mynode;
    opal_vpid_t vpid;
    char **list, **procs, **micro, *tmp, *regex;
    orte_job_t *dmns;
    orte_job_map_t *map;
    orte_app_context_t *app;
    orte_pmix_server_op_caddy_t *op;

    opal_output_verbose(2, orte_pmix_server_globals.output,
                        "%s orte:pmix:server setup job for %s",
                        ORTE_NAME_PRINT(ORTE_PROC_MY_NAME),
                        ORTE_JOBID_PRINT(jdata->jobid));

    /* setup the info list */
    info = OBJ_NEW(opal_list_t);

    /* jobid */
    kv = OBJ_NEW(opal_value_t);
    kv->key = strdup(OPAL_PMIX_JOBID);
    kv->data.string = strdup(ORTE_JOBID_PRINT(jdata->jobid));
    kv->type = OPAL_STRING;
    opal_list_append(info, &kv->super);

    /* assemble the node and proc map info */
    list = NULL;
    procs = NULL;
    map = jdata->map;
    for (i=0; i < map->nodes->size; i++) {
        micro = NULL;
        if (NULL != (node = (orte_node_t*)opal_pointer_array_get_item(map->nodes, i))) {
            opal_argv_append_nosize(&list, node->name);
            /* assemble all the ranks for this job that are on this node */
            for (k=0; k < node->procs->size; k++) {
                if (NULL != (pptr = (orte_proc_t*)opal_pointer_array_get_item(node->procs, k))) {
                    if (jdata->jobid == pptr->name.jobid) {
                        opal_argv_append_nosize(&micro, ORTE_VPID_PRINT(pptr->name.vpid));
                    }
                }
            }
            /* assemble the rank/node map */
            if (NULL != micro) {
                tmp = opal_argv_join(micro, ',');
                opal_argv_free(micro);
                opal_argv_append_nosize(&procs, tmp);
                free(tmp);
            }
        }
    }
    /* let the PMIx server generate the nodemap regex */
    if (NULL != list) {
        tmp = opal_argv_join(list, ',');
        opal_argv_free(list);
        list = NULL;
        if (OPAL_SUCCESS != (rc = opal_pmix.generate_regex(tmp, &regex))) {
            ORTE_ERROR_LOG(rc);
            free(tmp);
            OPAL_LIST_RELEASE(info);
            return rc;
        }
        free(tmp);
        kv = OBJ_NEW(opal_value_t);
        kv->key = strdup(OPAL_PMIX_NODE_MAP);
        kv->type = OPAL_STRING;
        kv->data.string = regex;
        opal_list_append(info, &kv->super);
    }

    /* let the PMIx server generate the procmap regex */
    if (NULL != procs) {
        tmp = opal_argv_join(procs, ';');
        opal_argv_free(procs);
        procs = NULL;
        if (OPAL_SUCCESS != (rc = opal_pmix.generate_ppn(tmp, &regex))) {
            ORTE_ERROR_LOG(rc);
            free(tmp);
            OPAL_LIST_RELEASE(info);
            return rc;
        }
        free(tmp);
        kv = OBJ_NEW(opal_value_t);
        kv->key = strdup(OPAL_PMIX_PROC_MAP);
        kv->type = OPAL_STRING;
        kv->data.string = regex;
        opal_list_append(info, &kv->super);
    }

    /* pass the number of nodes in the job */
    kv = OBJ_NEW(opal_value_t);
    kv->key = strdup(OPAL_PMIX_NUM_NODES);
    kv->type = OPAL_UINT32;
    kv->data.uint32 = map->num_nodes;
    opal_list_append(info, &kv->super);

    /* univ size */
    kv = OBJ_NEW(opal_value_t);
    kv->key = strdup(OPAL_PMIX_UNIV_SIZE);
    kv->type = OPAL_UINT32;
    kv->data.uint32 = jdata->total_slots_alloc;
    opal_list_append(info, &kv->super);

    /* job size */
    kv = OBJ_NEW(opal_value_t);
    kv->key = strdup(OPAL_PMIX_JOB_SIZE);
    kv->type = OPAL_UINT32;
    kv->data.uint32 = jdata->num_procs;
    opal_list_append(info, &kv->super);

    /* number of apps in this job */
    kv = OBJ_NEW(opal_value_t);
    kv->key = strdup(OPAL_PMIX_JOB_NUM_APPS);
    kv->type = OPAL_UINT32;
    kv->data.uint32 = jdata->num_apps;
    opal_list_append(info, &kv->super);

    /* max procs */
    kv = OBJ_NEW(opal_value_t);
    kv->key = strdup(OPAL_PMIX_MAX_PROCS);
    kv->type = OPAL_UINT32;
    kv->data.uint32 = jdata->total_slots_alloc;
    opal_list_append(info, &kv->super);

    /* setup the caddy for this operation */
    op = OBJ_NEW(orte_pmix_server_op_caddy_t);
    op->info = info;

    /* pass it down */
    rc = opal_pmix.server_setup_application(jdata->jobid, info, infocbfunc, op);
    if (ORTE_SUCCESS != rc) {
        OBJ_RELEASE(op);
    }
    return rc;
}

int orte_pmix_server_setup_drivers(orte_job_t *jdata,
                                   opal_buffer_t *buffer)
{

}
