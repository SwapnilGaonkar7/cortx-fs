/*
 * Filename:         efs.c
 * Description:      EOS file system interfaces

 * Do NOT modify or remove this copyright and confidentiality notice!
 * Copyright (c) 2019, Seagate Technology, LLC.
 * The code contained herein is CONFIDENTIAL to Seagate Technology, LLC.
 * Portions are also trade secret. Any use, duplication, derivation,
 * distribution or disclosure of this code, for any reason, not expressly
 * authorized is prohibited. All other rights are expressly reserved by
 * Seagate Technology, LLC.

 This file contains EFS file system interfaces.
*/

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <ini_config.h>
#include <common/log.h>
#include <common/helpers.h>
#include <dstore.h>
#include <dsal.h> /* dsal_init,fini */
#include <efs.h>
#include <internal/fs.h>
#include <debug.h>
#include <management.h>
#include <nsal.h> /* nsal_init,fini */

static struct collection_item *cfg_items;

static int efs_initialized;

int efs_init(const char *config_path, const struct efs_endpoint_ops *e_ops)
{
	struct collection_item *errors = NULL;
	int rc = 0;
	struct collection_item *item = NULL;
        char *log_path = NULL;
        char *log_level = NULL;

	/** only initialize efs once */
	if (__sync_fetch_and_add(&efs_initialized, 1)) {
		return 0;
	}

	rc = config_from_file("libkvsns", config_path, &cfg_items,
			      INI_STOP_ON_ERROR, &errors);
	if (rc) {
		free_ini_config_errors(errors);
		rc = -rc;
		goto out;
	}

	RC_WRAP(get_config_item, "log", "path", cfg_items, &item);

	/** Path not specified, use default */
	if (item == NULL) {
		log_path = "/var/log/cortx/fs/cortxfs.log";
	} else {
		log_path = get_string_config_value(item, NULL);
	}
	item = NULL;

	RC_WRAP(get_config_item, "log", "level", cfg_items, &item);
	if (item == NULL) {
		log_level = "LEVEL_INFO";
	} else {
		log_level = get_string_config_value(item, NULL);
	}

	rc = log_init(log_path, log_level_no(log_level));
        if (rc != 0) {
                rc = -EINVAL;
                goto out;
        }
	rc = utils_init(cfg_items);
	if (rc != 0) {
		log_err("utils_init failed, rc=%d", rc);
                goto log_cleanup;
        }
	rc = nsal_module_init(cfg_items);
        if (rc) {
                log_err("nsal_init failed, rc=%d", rc);
                goto utils_cleanup;
        }
	rc = dsal_init(cfg_items, 0);
	if (rc) {
		log_err("dsal_init failed, rc=%d", rc);
		goto nsal_cleanup;
	}
	rc = efs_fs_init(e_ops);
	if (rc) {
		log_err("efs_fs_init failed, rc=%d", rc);
		goto dsal_cleanup;
	}
	rc = management_init();
	if (rc) {
		log_err("management_init failed, rc=%d", rc);
                goto efs_fs_cleanup;
        }
	goto out;
efs_fs_cleanup:
	efs_fs_fini();
dsal_cleanup:
	dsal_fini();
nsal_cleanup:
	nsal_module_fini();
utils_cleanup:
	utils_fini();
log_cleanup:
	log_fini();
out:
	if (rc) {
		free_ini_config_errors(errors);
		return rc;
	}

	/** @todo : remove all existing opened FD (crash recovery) */
	return rc;
}

int efs_fini(void)
{
	int rc = 0;
	rc = management_fini();
	if (rc) {
		log_err("management_fini failed, rc=%d", rc);
        }
	rc = efs_fs_fini();
	if (rc) {
                log_err("efs_fs_fini failed, rc=%d", rc);
        }
	rc = dsal_fini();
	if (rc) {
                log_err("dsal_fini failed, rc=%d", rc);
        }
	rc = nsal_module_fini();
	if (rc) {
                log_err("nsal_fini failed, rc=%d", rc);
        }
	rc = utils_fini();
	if (rc) {
        	log_err("utils_fini failed, rc=%d", rc);
        }
	rc = log_fini();
	if (rc) {
                log_err("log_fini failed, rc=%d ", rc);
        }
	free_ini_config_errors(cfg_items);
        return rc;
}