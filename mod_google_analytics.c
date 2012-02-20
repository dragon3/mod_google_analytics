/*
$ perldoc mod_google_analytics.c
 
=encoding utf8

=head1 NAME
 
mod_google_analytics.c -- Apache mod_google_analytics module

=head1 SYNOPSIS

 LoadModule google_analytics_module modules/mod_google_analytics.so

 AddOutputFilterByType GOOGLE_ANALYTICS text/html
 GoogleAnalyticsAccountNumber UA-1234567-8
 GoogleAnalyticsMobileAccountNumber MO-1234567-8

=head1 AUTHOR

Ryuzo Yamamoto (dragon3)

=head1 LICENSE

Copyright 2008-2012 Ryuzo Yamamoto

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

// rand
#include <stdlib.h>

#define VERSION "0.3"

static const char *google_analytics_filter_name = "GOOGLE_ANALYTICS";
static const char *body_end_tag = "</body>";
static const unsigned int body_end_tag_length = 7;
static const char *replace_base = "<script type=\"text/javascript\"><!-- \n var gaJsHost = ((\"https:\" == document.location.protocol) ? \"https://ssl.\" : \"http://www.\");document.write(unescape(\"%%3Cscript src='\" + gaJsHost + \"google-analytics.com/ga.js' type='text/javascript'%%3E%%3C/script%%3E\"));\n//--></script><script type=\"text/javascript\"><!-- \n try {var pageTracker = _gat._getTracker(\"%s\");pageTracker._trackPageview();} catch(err) {}; \n//--></script></body>";
static const char *mobile_replace_base = "<img src=\"/ga.php?utmac=%s&amp;utmn=%d&amp;utmr=%s&amp;utmp=%s&amp;guid=ON\" />";
static const char *mobile_ua_pattern = "DoCoMo|J-PHONE|Vodafone|SoftBank|DDIPOCKET|WILLCOM|emobile|KDDI";
static const char *tag_exists = "google-analytics\\.com/(ga|urchin)\\.js";
static const ap_regex_t *regex_tag_exists;
static const ap_regex_t *regex_ua_mobile;
static const apr_strmatch_pattern *pattern_body_end_tag;

static int rand_init_done = 0;

module AP_MODULE_DECLARE_DATA google_analytics_module;

typedef struct {
    char *account_number;
    char *replace;
    char *mobile_account_number;
} google_analytics_filter_config;

typedef struct {
    apr_bucket_brigade *bbsave;
} google_analytics_filter_ctx;

static const int is_mobile(request_rec *r)
{
    const char *ua = apr_table_get(r->headers_in, "User-Agent");
    if (ap_regexec(regex_ua_mobile, ua, 0, NULL, 0) == 0) {
        return 1;
    }
    return 0;
}

static const char * create_mobile_replace_tag(request_rec *r, google_analytics_filter_config *c)
{
    if (!rand_init_done) {
        srand((unsigned)(getpid()));
        rand_init_done = 1;
    }

    const char *referer = apr_table_get(r->headers_in, "Referer");
    // utmn, utmr, utmp = rand, referer, path
    return apr_psprintf(r->pool, mobile_replace_base,
                        c->mobile_account_number,
                        rand(),
                        ap_escape_uri(r->pool, referer ? referer : "-"),
                        ap_escape_uri(r->pool, r->uri));
}

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

    const char * mobile_replace_tag;
    
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
                if (is_mobile(r) == 1 && c->mobile_account_number) {
                    mobile_replace_tag = create_mobile_replace_tag(r, c);
                    b1 = apr_bucket_immortal_create(mobile_replace_tag, strlen(mobile_replace_tag),
                                                    r->connection->bucket_alloc);
                } else {
                    b1 = apr_bucket_immortal_create(c->replace, strlen(c->replace),
                                                    r->connection->bucket_alloc);
                }
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

    // mobile
    c->mobile_account_number = overrides->mobile_account_number ? overrides->mobile_account_number : base->mobile_account_number;

    return c;
}

static const char * set_account_number(cmd_parms *cmd, void *mconfig, const char *arg)
{
    google_analytics_filter_config *c = (google_analytics_filter_config *)mconfig;
    
    c->account_number = apr_pstrdup(cmd->pool, arg);
    c->replace = apr_psprintf(cmd->pool, replace_base, c->account_number);
    
    return NULL;
}

static const char * set_mobile_account_number(cmd_parms *cmd, void *mconfig, const char *arg)
{
    google_analytics_filter_config *c = (google_analytics_filter_config *)mconfig;

    c->mobile_account_number = apr_pstrdup(cmd->pool, arg);

    return NULL;
}

static int google_analytics_post_config(apr_pool_t *p, apr_pool_t *plog,
                                        apr_pool_t *ptemp, server_rec *s)
{
    // gaタグ存在するかどうかの検索 regex
    regex_tag_exists = ap_pregcomp(p, tag_exists, (AP_REG_EXTENDED | AP_REG_ICASE | AP_REG_NOSUB));
    ap_assert(regex_tag_exists != NULL);

    // </body>タグの検索 regex
    pattern_body_end_tag = apr_strmatch_precompile(p, body_end_tag, 0);

    // 携帯端末 User-Agent 判定のための regex
    regex_ua_mobile = ap_pregcomp(p, mobile_ua_pattern, (AP_REG_EXTENDED | AP_REG_ICASE | AP_REG_NOSUB));
    ap_assert(regex_ua_mobile != NULL);

    return OK;
}

static const command_rec cmds[] = {
    AP_INIT_TAKE1("GoogleAnalyticsAccountNumber", set_account_number, NULL, OR_FILEINFO,
                  "Set the Google Analytics account number."),
    AP_INIT_TAKE1("GoogleAnalyticsMobileAccountNumber", set_mobile_account_number, NULL, OR_FILEINFO,
                  "Set the Google Analytics account number for mobile tracking."),
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

