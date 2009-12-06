/*
$ perldoc mod_google_analytics.c
 
=encoding utf8

=head1 NAME
 
mod_google_analytics.c -- Apache mod_google_analytics module

=head1 SYNOPSIS

 LoadModule google_analytics_module modules/mod_google_analytics.so

 AddOutputFilterByType GOOGLE_ANALYTICS text/html
 GoogleAnalyticsAccountNumber UA-1234567-8

=head1 AUTHOR

Ryuzo Yamamoto (dragon3)

=head1 LICENSE

Copyright 2008 Ryuzo Yamamoto

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

=cut
*/

#include "httpd.h"
#include "http_config.h"
#include "util_filter.h"
#include "ap_config.h"
#include "ap_regex.h"

#include "apr_strmatch.h"
#include "apr_strings.h"

#define VERSION "0.2-async"

static const char *google_analytics_filter_name = "GOOGLE_ANALYTICS";
static const char *body_end_tag = "</body>";
static const unsigned int body_end_tag_length = 7;
static const char *replace_base = "<script type=\"text/javascript\"><!--\n  var _gaq = _gaq || [];\n _gaq.push(['_setAccount', '%s']);\n _gaq.push(['_trackPageview']);\n (function() {\n    var ga = document.createElement('script');\n ga.src = ('https:' == document.location.protocol ? 'https://ssl' : 'http://www') + '.google-analytics.com/ga.js';\n ga.setAttribute('async', 'true');\n document.documentElement.firstChild.appendChild(ga);\n })();\n--></script>\n</head>";
static const char *tag_exists = "google-analytics\\.com/(ga|urchin)\\.js";
static const ap_regex_t *regex_tag_exists;
static const apr_strmatch_pattern *pattern_body_end_tag;

module AP_MODULE_DECLARE_DATA google_analytics_module;

typedef struct {
    char *account_number;
    char *replace;
} google_analytics_filter_config;

typedef struct {
    apr_bucket_brigade *bbsave;
} google_analytics_filter_ctx;

static apr_status_t google_analytics_out_filter(ap_filter_t *f, apr_bucket_brigade *bb)
{
    request_rec *r = f->r;
    google_analytics_filter_ctx *ctx = f->ctx;
    google_analytics_filter_config *c;

    apr_bucket *b = APR_BRIGADE_FIRST(bb);

    apr_size_t bytes;
    apr_size_t fbytes;
    apr_size_t offs;
    const char *buf;
    const char *le = NULL;
    const char *le_n;
    const char *le_r;

    const char *bufp;
    const char *subs;
    unsigned int match;

    apr_bucket *b1;
    char *fbuf;
    int found = 0;
    apr_status_t rv;

    apr_bucket_brigade *bbline;
    
    // サブリクエストならなにもしない
    if (r->main) {
        ap_remove_output_filter(f);
        return ap_pass_brigade(f->next, bb);
    }

    c = ap_get_module_config(r->per_dir_config, &google_analytics_module);

    if (ctx == NULL) {
        ctx = f->ctx = apr_pcalloc(r->pool, sizeof(google_analytics_filter_ctx));
        ctx->bbsave = apr_brigade_create(r->pool, f->c->bucket_alloc);
    }

    // length かわってしまうので unset で OK?
    apr_table_unset(r->headers_out, "Content-Length");
    apr_table_unset(r->headers_out, "Content-MD5");
    apr_table_unset(r->headers_out, "Accept-Ranges");
    apr_table_unset(r->headers_out, "ETag");

    bbline = apr_brigade_create(r->pool, f->c->bucket_alloc);
    
    // 改行毎なbucketに編成しなおす
    while ( b != APR_BRIGADE_SENTINEL(bb) ) {
        if ( !APR_BUCKET_IS_METADATA(b) ) {
            if ( apr_bucket_read(b, &buf, &bytes, APR_BLOCK_READ) == APR_SUCCESS ) {
                if ( bytes == 0 ) {
                    APR_BUCKET_REMOVE(b);
                } else {
					while ( bytes > 0 ) {
						le_n = memchr(buf, '\n', bytes);
                        le_r = memchr(buf, '\r', bytes);
                        if ( le_n != NULL ) {
                            if ( le_n == le_r + sizeof(char)) {
                                le = le_n;
                            }
                            else if ( (le_r < le_n) && (le_r != NULL) ) {
                                le = le_r;
                            }
                            else {
                                le = le_n;
                            }
                        }
                        else {
                            le = le_r;
                        }

                        if ( le ) {
                            offs = 1 + ((unsigned int)le-(unsigned int)buf) / sizeof(char);
                            apr_bucket_split(b, offs);
                            bytes -= offs;
                            buf += offs;
                            b1 = APR_BUCKET_NEXT(b);
                            APR_BUCKET_REMOVE(b);

                            if ( !APR_BRIGADE_EMPTY(ctx->bbsave) ) {
                                APR_BRIGADE_INSERT_TAIL(ctx->bbsave, b);
                                rv = apr_brigade_pflatten(ctx->bbsave, &fbuf, &fbytes, r->pool);
                                b = apr_bucket_pool_create(fbuf, fbytes, r->pool,
                                                           r->connection->bucket_alloc);
                                apr_brigade_cleanup(ctx->bbsave);
                            }
                            APR_BRIGADE_INSERT_TAIL(bbline, b);
                            b = b1;
                        } else {
                            APR_BUCKET_REMOVE(b);
                            APR_BRIGADE_INSERT_TAIL(ctx->bbsave, b);
                            bytes = 0;
                        }
                    } /* while bytes > 0 */
				}
            } else {
                APR_BUCKET_REMOVE(b);
            }
        } else if ( APR_BUCKET_IS_EOS(b) ) {
            if ( !APR_BRIGADE_EMPTY(ctx->bbsave) ) {
                rv = apr_brigade_pflatten(ctx->bbsave, &fbuf, &fbytes, r->pool);
                b1 = apr_bucket_pool_create(fbuf, fbytes, r->pool,
                                            r->connection->bucket_alloc);
                APR_BRIGADE_INSERT_TAIL(bbline, b1);
            }
            apr_brigade_cleanup(ctx->bbsave);
            f->ctx = NULL;
            APR_BUCKET_REMOVE(b);
            APR_BRIGADE_INSERT_TAIL(bbline, b);
        } else {
            apr_bucket_delete(b);
        }
        b = APR_BRIGADE_FIRST(bb);
    }

    // 改行毎なbucketをまわす
    for ( b = APR_BRIGADE_FIRST(bbline);
          b != APR_BRIGADE_SENTINEL(bbline);
          b = APR_BUCKET_NEXT(b) ) {
        if ( !APR_BUCKET_IS_METADATA(b)
             && (apr_bucket_read(b, &buf, &bytes, APR_BLOCK_READ) == APR_SUCCESS)) {

            bufp = buf;

			if (ap_regexec(regex_tag_exists, bufp, 0, NULL, 0) == 0) {
				break;
			}
			
            subs = apr_strmatch(pattern_body_end_tag, bufp, bytes);
            if (subs != NULL) {
                match = ((unsigned int)subs - (unsigned int)bufp) / sizeof(char);
                bytes -= match;
                bufp += match;
                apr_bucket_split(b, match);
                b1 = APR_BUCKET_NEXT(b);
                apr_bucket_split(b1, body_end_tag_length);
                b = APR_BUCKET_NEXT(b1);
                apr_bucket_delete(b1);
                bytes -= body_end_tag_length;
                bufp += body_end_tag_length;
                b1 = apr_bucket_immortal_create(c->replace, strlen(c->replace),
                                                r->connection->bucket_alloc);
                APR_BUCKET_INSERT_BEFORE(b, b1);
            }
        }
    }
    rv = ap_pass_brigade(f->next, bbline);

    for ( b = APR_BRIGADE_FIRST(ctx->bbsave);
          b != APR_BRIGADE_SENTINEL(ctx->bbsave);
          b = APR_BUCKET_NEXT(b)) {
        apr_bucket_setaside(b, r->pool);
    }

    return rv;
}

static void * create_dir_config(apr_pool_t *p, char *dir)
{
    google_analytics_filter_config *c = apr_pcalloc(p, sizeof(google_analytics_filter_config));
    return (void *)c;
}

static void * merge_dir_config(apr_pool_t *p, void *basev, void *overridesv)
{
    google_analytics_filter_config *c = apr_palloc(p, sizeof(google_analytics_filter_config));
    google_analytics_filter_config *base = (google_analytics_filter_config *)basev;
    google_analytics_filter_config *overrides = (google_analytics_filter_config *)overridesv;

    c->account_number = overrides->account_number ? overrides->account_number : base->account_number;
    c->replace = overrides->replace ? overrides->replace : base->replace;
    return c;
}

static const char * set_account_number(cmd_parms *cmd, void *mconfig, const char *arg)
{
    google_analytics_filter_config *c = (google_analytics_filter_config *)mconfig;
    
    // TODO check arg
    // return "specify your account number.";
    
    c->account_number = apr_pstrdup(cmd->pool, arg);
    c->replace = apr_psprintf(cmd->pool, replace_base, c->account_number);
    
    return NULL;
}

static int google_analytics_post_config(apr_pool_t *p, apr_pool_t *plog,
										apr_pool_t *ptemp, server_rec *s)
{
	regex_tag_exists = ap_pregcomp(p, tag_exists, (AP_REG_EXTENDED | AP_REG_ICASE | AP_REG_NOSUB));
	ap_assert(regex_tag_exists != NULL);

	pattern_body_end_tag = apr_strmatch_precompile(p, body_end_tag, 0);
	return OK;
}

static const command_rec cmds[] = {
    AP_INIT_TAKE1("GoogleAnalyticsAccountNumber", set_account_number, NULL, OR_FILEINFO,
                  "Set the Google Analytics account number."),
    {NULL}
};
  
static void register_hooks(apr_pool_t *p)
{
    ap_hook_post_config(google_analytics_post_config, NULL, NULL, APR_HOOK_MIDDLE);
    ap_register_output_filter(google_analytics_filter_name, google_analytics_out_filter, NULL,
                              AP_FTYPE_RESOURCE);
}

module AP_MODULE_DECLARE_DATA google_analytics_module = {
    STANDARD20_MODULE_STUFF, 
    create_dir_config,     /* create per-dir    config structures */
    merge_dir_config,      /* merge  per-dir    config structures */
    NULL,                  /* create per-server config structures */
    NULL,                  /* merge  per-server config structures */
    cmds,                  /* table of config file commands       */
    register_hooks         /* register hooks */
};

