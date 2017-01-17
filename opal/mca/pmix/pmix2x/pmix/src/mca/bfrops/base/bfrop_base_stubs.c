/*
 * Copyright (c) 2004-2005 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2006 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart,
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 * Copyright (c) 2015-2017 Intel, Inc.  All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#include <src/include/pmix_config.h>


#include <stdio.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "src/util/argv.h"
#include "src/util/error.h"
#include "src/include/pmix_globals.h"

#include "src/mca/bfrops/base/base.h"

char* pmix_bfrops_stub_get_available_modules(void)
{
    pmix_bfrops_base_active_module_t *active;
    char **tmp=NULL, *reply=NULL;

    if (!pmix_bfrops_globals.initialized) {
        return NULL;
    }

    PMIX_LIST_FOREACH(active, &pmix_bfrops_globals.actives, pmix_bfrops_base_active_module_t) {
        pmix_argv_append_nosize(&tmp, active->component->base.pmix_mca_component_name);
    }
    if (NULL != tmp) {
        reply = pmix_argv_join(tmp, ',');
        pmix_argv_free(tmp);
    }
    return reply;
}

pmix_status_t pmix_bfrops_stub_assign_module(struct pmix_peer_t *peer,
                                             const char *version)
{
    pmix_peer_t *pr = (pmix_peer_t*)peer;
    pmix_bfrops_base_active_module_t *active;
    pmix_bfrops_module_t *mod;

    if (!pmix_bfrops_globals.initialized) {
        return PMIX_ERR_INIT;
    }

    PMIX_LIST_FOREACH(active, &pmix_bfrops_globals.actives, pmix_bfrops_base_active_module_t) {
        if (NULL != (mod = active->component->assign_module(version))) {
            pr->compat.bfrops = mod;
            return PMIX_SUCCESS;
        }
    }

    /* we only get here if nothing was found */
    return PMIX_ERR_NOT_FOUND;
}

pmix_buffer_t* pmix_bfrops_stub_create_buffer(struct pmix_peer_t *pr)
{
    pmix_buffer_t *bf;
    pmix_peer_t *peer = (pmix_peer_t*)pr;

    bf = PMIX_NEW(pmix_buffer_t);
    if (NULL != bf) {
        bf->type = peer->compat.type;
    }
    return bf;
}

void pmix_bfrops_construct_buffer(struct pmix_peer_t *pr,
                                  pmix_buffer_t *buf)
{
    pmix_peer_t *peer = (pmix_peer_t*)pr;

    PMIX_CONSTRUCT(buf, pmix_buffer_t);
    buf->type = peer->compat.type;
}

pmix_status_t pmix_bfrops_stub_pack(struct pmix_peer_t *pr,
                                    pmix_buffer_t *buffer,
                                    const void *src,
                                    int32_t num_values,
                                    pmix_data_type_t type)
{
    pmix_peer_t *peer = (pmix_peer_t*)pr;
    return peer->compat.bfrops->pack(buffer, src, num_values, type);
}

pmix_status_t pmix_bfrops_stub_unpack(struct pmix_peer_t *pr,
                                      pmix_buffer_t *buffer, void *dest,
                                      int32_t *max_num_values,
                                      pmix_data_type_t type)
{
    pmix_peer_t *peer = (pmix_peer_t*)pr;
    return peer->compat.bfrops->unpack(buffer, dest, max_num_values, type);
}

pmix_status_t pmix_bfrops_stub_copy(struct pmix_peer_t *pr,
                                    void **dest, void *src,
                                    pmix_data_type_t type)
{
    pmix_peer_t *peer = (pmix_peer_t*)pr;
    return peer->compat.bfrops->copy(dest, src, type);
}

pmix_status_t pmix_bfrops_stub_print(struct pmix_peer_t *pr,
                                     char **output, char *prefix,
                                     void *src, pmix_data_type_t type)
{
    pmix_peer_t *peer = (pmix_peer_t*)pr;
    return peer->compat.bfrops->print(output, prefix, src, type);
}

pmix_status_t pmix_bfrops_stub_copy_payload(struct pmix_peer_t *pr,
                                            pmix_buffer_t *dest,
                                            pmix_buffer_t *src)
{
    pmix_peer_t *peer = (pmix_peer_t*)pr;
    return peer->compat.bfrops->copy_payload(dest, src);
}

pmix_status_t pmix_bfrops_stub_value_xfer(struct pmix_peer_t *pr,
                                          pmix_value_t *dest,
                                          pmix_value_t *src)
{
    pmix_peer_t *peer = (pmix_peer_t*)pr;
    return peer->compat.bfrops->value_xfer(dest, src);
}

void pmix_bfrops_stub_value_load(struct pmix_peer_t *pr,
                                 pmix_value_t *v, void *data,
                                 pmix_data_type_t type)
{
    pmix_peer_t *peer = (pmix_peer_t*)pr;
    peer->compat.bfrops->value_load(v, data, type);
}

pmix_status_t pmix_bfrops_stub_value_unload(struct pmix_peer_t *pr,
                                            pmix_value_t *kv, void **data,
                                            size_t *sz, pmix_data_type_t type)
{
    pmix_peer_t *peer = (pmix_peer_t*)pr;
    return peer->compat.bfrops->value_unload(kv, data, sz, type);
}

pmix_value_cmp_t pmix_bfrops_stub_value_cmp(struct pmix_peer_t *pr,
                                            pmix_value_t *p1, pmix_value_t *p2)
{
    pmix_peer_t *peer = (pmix_peer_t*)pr;
    return peer->compat.bfrops->value_cmp(p1, p2);
}

pmix_status_t pmix_bfrops_stub_register_type(struct pmix_peer_t *pr,
                                             const char *name, pmix_data_type_t type,
                                             pmix_bfrop_pack_fn_t pack,
                                             pmix_bfrop_unpack_fn_t unpack,
                                             pmix_bfrop_copy_fn_t copy,
                                             pmix_bfrop_print_fn_t print)
{
    pmix_peer_t *peer = (pmix_peer_t*)pr;
    return peer->compat.bfrops->register_type(name, type,
                               pack, unpack,
                               copy, print);
}

const char* pmix_bfrops_stub_data_type_string(struct pmix_peer_t *pr,
                                              pmix_data_type_t type)
{
    pmix_peer_t *peer = (pmix_peer_t*)pr;
    return peer->compat.bfrops->data_type_string(type);
}
