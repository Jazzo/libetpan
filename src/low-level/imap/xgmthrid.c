/*
 * libEtPan! -- a mail stuff library
 *
 * Copyright (C) 2001, 2014 - DINH Viet Hoa
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the libEtPan! project nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#include "xgmthrid.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#include "clist.h"
#include "mailimap_types_helper.h"
#include "mailimap_extension.h"
#include "mailimap_keywords.h"
#include "mailimap_parser.h"
#include "mailimap_sender.h"
#include "mailimap.h"

struct mailimap_fetch_att * mailimap_fetch_att_new_xgmthrid(void)
{
  char * keyword;
  struct mailimap_fetch_att * att;

  keyword = strdup("X-GM-THRID");
  if (keyword == NULL)
    return NULL;

  att = mailimap_fetch_att_new_extension(keyword);
  if (att == NULL) {
    free(keyword);
    return NULL;
  }
  
  return att;
}

int mailimap_has_xgmthrid(mailimap * session)
{
  return mailimap_has_extension(session, "X-GM-EXT-1");
}

static int
mailimap_xgmthrid_extension_parse(int calling_parser, mailstream * fd,
                                   MMAPString * buffer, size_t * indx,
                                   struct mailimap_extension_data ** result,
                                   size_t progr_rate, progress_function * progr_fun);

static void
mailimap_xgmthrid_extension_data_free(struct mailimap_extension_data * ext_data);

LIBETPAN_EXPORT
struct mailimap_extension_api mailimap_extension_xgmthrid = {
    /* name */          "X-GM-THRID",
    /* extension_id */  MAILIMAP_EXTENSION_XGMTHRID,
    /* parser */        mailimap_xgmthrid_extension_parse,
    /* free */          mailimap_xgmthrid_extension_data_free
};

static int fetch_data_xgmthrid_parse(mailstream * fd,
                                      MMAPString * buffer, size_t * indx,
                                      char ** result, size_t progr_rate, progress_function * progr_fun)
{
    size_t cur_token;
    char  *thrid_str;
    int r;
    
    cur_token = * indx;

    r = mailimap_token_case_insensitive_parse(fd, buffer,
                                              &cur_token, "X-GM-THRID");
    if (r != MAILIMAP_NO_ERROR)
        return r;

    r = mailimap_space_parse(fd, buffer, &cur_token);
    if (r != MAILIMAP_NO_ERROR)
        return r;

    r = mailimap_astring_parse(fd, buffer, &cur_token, &thrid_str, progr_rate, progr_fun);
    if (r != MAILIMAP_NO_ERROR)
        return r;

    r = mailimap_space_parse(fd, buffer, &cur_token);
    if (r != MAILIMAP_NO_ERROR)
        return r;

    * indx = cur_token;
    * result = thrid_str;
    
    return MAILIMAP_NO_ERROR;
}

static int
mailimap_xgmthrid_extension_parse(int calling_parser, mailstream * fd,
                                   MMAPString * buffer, size_t * indx,
                                   struct mailimap_extension_data ** result,
                                   size_t progr_rate, progress_function * progr_fun)
{
    size_t cur_token;
    char * thrid;
    struct mailimap_extension_data * ext_data;
    int r;
    
    cur_token = * indx;

    switch (calling_parser)
    {
        case MAILIMAP_EXTENDED_PARSER_FETCH_DATA:
            
            r = fetch_data_xgmthrid_parse(fd, buffer, &cur_token, &thrid, progr_rate, progr_fun);
            if (r != MAILIMAP_NO_ERROR)
              return r;

            ext_data = mailimap_extension_data_new(&mailimap_extension_xgmthrid,
                                                   MAILIMAP_XGMTHRID_TYPE_THRID, thrid);
            if (ext_data == NULL) {
                return MAILIMAP_ERROR_MEMORY;
            }

            * result = ext_data;
            * indx = cur_token;

            return MAILIMAP_NO_ERROR;
            
        default: {
            return MAILIMAP_ERROR_PARSE;
        }
    }
}

static void
mailimap_xgmthrid_extension_data_free(struct mailimap_extension_data * ext_data)
{
    if (ext_data == NULL)
        return;
 
    free(ext_data);
}

int mailimap_fetch_xgmthrid(mailimap * session,
                            struct mailimap_set * set,
                            clist ** results)
{
    struct mailimap_response * response;
    int r;
    int error_code;
    
    if (session->imap_state != MAILIMAP_STATE_SELECTED)
        return MAILIMAP_ERROR_BAD_STATE;
    
    r = mailimap_send_current_tag(session);
    if (r != MAILIMAP_NO_ERROR)
        return r;
    
    r = mailimap_token_send(session->imap_stream, "FETCH");
    if (r != MAILIMAP_NO_ERROR)
        return r;
    r = mailimap_space_send(session->imap_stream);
    if (r != MAILIMAP_NO_ERROR)
        return r;
    r = mailimap_set_send(session->imap_stream, set);
    if (r != MAILIMAP_NO_ERROR)
        return r;
    r = mailimap_space_send(session->imap_stream);
    if (r != MAILIMAP_NO_ERROR)
        return r;
    
    r = mailimap_token_send(session->imap_stream, "(X-GM-THRID)");
    if (r != MAILIMAP_NO_ERROR)
        return r;
    
    r = mailimap_crlf_send(session->imap_stream);
    if (r != MAILIMAP_NO_ERROR)
      return r;
    
    if (mailstream_flush(session->imap_stream) == -1)
        return MAILIMAP_ERROR_STREAM;
    
    if (mailimap_read_line(session) == NULL)
        return MAILIMAP_ERROR_STREAM;
   
    r = mailimap_parse_response(session, &response);
    if (r != MAILIMAP_NO_ERROR)
        return r;
    
    error_code = response->rsp_resp_done->rsp_data.rsp_tagged->rsp_cond_state->rsp_type;
    
    *results = response->rsp_cont_req_or_resp_data_list;

    mailimap_response_free(response);
    
    switch (error_code) {
        case MAILIMAP_RESP_COND_STATE_OK:
            return MAILIMAP_NO_ERROR;
            
        default:
            return MAILIMAP_ERROR_FETCH;
    }
}
