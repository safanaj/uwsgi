#include "psgi.h" 

extern struct uwsgi_server uwsgi;
struct uwsgi_perl uperl;

extern struct uwsgi_plugin psgi_plugin;


XS(XS_input_seek) {

        dXSARGS;
	struct wsgi_request *wsgi_req = current_wsgi_req();

        psgi_check_args(1);
	uwsgi_request_body_seek(wsgi_req, SvIV(ST(0)));

        XSRETURN(0);
}

XS(XS_error) {
	dXSARGS;
	struct wsgi_request *wsgi_req = current_wsgi_req();
	struct uwsgi_app *wi = &uwsgi_apps[wsgi_req->app_id];

        psgi_check_args(0);

	if (uwsgi.threads > 1) {
        	ST(0) = sv_bless(newRV(sv_newmortal()), ((HV **)wi->error)[wsgi_req->async_id]);
	}
	else {
        	ST(0) = sv_bless(newRV(sv_newmortal()), ((HV **)wi->error)[0]);
	}
        XSRETURN(1);
}

XS(XS_input) {

        dXSARGS;
	struct wsgi_request *wsgi_req = current_wsgi_req();
	struct uwsgi_app *wi = &uwsgi_apps[wsgi_req->app_id];
        psgi_check_args(0);

	if (uwsgi.threads > 1) {
        	ST(0) = sv_bless(newRV(sv_newmortal()), ((HV **)wi->input)[wsgi_req->async_id]);
	}
	else {
        	ST(0) = sv_bless(newRV(sv_newmortal()), ((HV **)wi->input)[0]);
	}
        XSRETURN(1);
}

XS(XS_psgix_logger) {
	dXSARGS;
	psgi_check_args(1);
	HV *hv_args = (HV *) (SvRV(ST(0)));
	if (!hv_exists(hv_args, "level", 5) || !hv_exists(hv_args, "message", 7)) {
		Perl_croak(aTHX_ "psgix.logger requires bot level and message items");
	}
	char *level = SvPV_nolen(*(hv_fetch(hv_args, "level", 5, 0)));
	char *message = SvPV_nolen(*(hv_fetch(hv_args, "message", 7, 0)));
	uwsgi_log("[uwsgi-perl %s] %s\n", level, message); 
	XSRETURN(0);
}

XS(XS_stream)
{
    dXSARGS;
    struct wsgi_request *wsgi_req = current_wsgi_req();
    struct uwsgi_app *wi = &uwsgi_apps[wsgi_req->app_id];

    psgi_check_args(1);

    AV *response = (AV* ) SvREFCNT_inc(SvRV(ST(0))) ;

	if (av_len(response) == 2) {
		while (psgi_response(wsgi_req, response) != UWSGI_OK);
	}
	else if (av_len(response) == 1) {
		while (psgi_response(wsgi_req, response) != UWSGI_OK);

		SvREFCNT_dec(response);
		if (uwsgi.threads > 1) {
                	ST(0) = sv_bless(newRV(sv_newmortal()), ((HV **)wi->stream)[wsgi_req->async_id]);
		}
		else {
                	ST(0) = sv_bless(newRV(sv_newmortal()), ((HV **)wi->stream)[0]);
		}
                XSRETURN(1);
	}
	else {
		uwsgi_log("invalid PSGI response: array size %d\n", av_len(response));
	}

	SvREFCNT_dec(response);
	XSRETURN(0);

}


XS(XS_input_read) {

        dXSARGS;
        struct wsgi_request *wsgi_req = current_wsgi_req();

        psgi_check_args(3);

        SV *read_buf = ST(1);
        unsigned long arg_len = SvIV(ST(2));

	ssize_t rlen = 0;

	char *buf = uwsgi_request_body_read(wsgi_req, arg_len, &rlen);
        if (buf) {
		sv_setpvn(read_buf, buf, rlen);
		goto ret;
        }

        // error ?
        if (rlen < 0) {
		croak("error during read(%lu) on psgi.input", arg_len);
		goto ret;
        }

	croak("timeout during read(%lu) on psgi.input", arg_len);

ret:
        XSRETURN_IV(rlen);
}


XS(XS_streaming_close) {

        dXSARGS;
        psgi_check_args(0);

        XSRETURN(0);
}

XS(XS_streaming_write) {

        dXSARGS;
        struct wsgi_request *wsgi_req = current_wsgi_req();
        STRLEN blen;
        char *body;

        psgi_check_args(2);

        body = SvPV(ST(1), blen);

	uwsgi_response_write_body_do(wsgi_req, body, blen);
	uwsgi_pl_check_write_errors {
		croak("error while streaming PSGI response");
	}

        XSRETURN(0);
}

XS(XS_error_print) {

	dXSARGS;
        STRLEN blen;
        char *body;

        psgi_check_args(1);

	if (items > 1) {
        	body = SvPV(ST(1), blen);
		uwsgi_log("%.*s", (int) blen, body);
	}

        XSRETURN(0);
}

XS(XS_uwsgi_stacktrace) {

	dXSARGS;

        psgi_check_args(0);
	uwsgi_log("%s", SvPV_nolen(ERRSV));
	uwsgi_log("*** uWSGI perl stacktrace ***\n");
	SV *ret = perl_eval_pv("Devel::StackTrace->new->as_string;", 0);
        uwsgi_log("%s", SvPV_nolen(ret));
	uwsgi_log("*** end of perl stacktrace ***\n\n");
	XSRETURN(0);

}


/* automatically generated */

EXTERN_C void xs_init (pTHX);

EXTERN_C void boot_DynaLoader (pTHX_ CV* cv);

        EXTERN_C void
xs_init(pTHX)
{
        char *file = __FILE__;
        dXSUB_SYS;

        /* DynaLoader is a special case */
        newXS("DynaLoader::boot_DynaLoader", boot_DynaLoader, file);

	if (!uperl.tmp_input_stash) goto nonworker;

        newXS("uwsgi::input::new", XS_input, "uwsgi::input");
        newXS("uwsgi::input::read", XS_input_read, "uwsgi::input");
        newXS("uwsgi::input::seek", XS_input_seek, "uwsgi::input");

        uperl.tmp_input_stash[uperl.tmp_current_i] = gv_stashpv("uwsgi::input", 0);

        newXS("uwsgi::error::new", XS_error, "uwsgi::error");
        newXS("uwsgi::error::print", XS_error_print, "uwsgi::print");
        uperl.tmp_error_stash[uperl.tmp_current_i] = gv_stashpv("uwsgi::error", 0);
	uperl.tmp_psgix_logger[uperl.tmp_current_i] = newXS("uwsgi::psgix_logger", XS_psgix_logger, "uwsgi");
        uperl.tmp_stream_responder[uperl.tmp_current_i] = newXS("uwsgi::stream", XS_stream, "uwsgi");

        newXS("uwsgi::streaming::write", XS_streaming_write, "uwsgi::streaming");
        newXS("uwsgi::streaming::close", XS_streaming_close, "uwsgi::streaming");

        newXS("uwsgi::stacktrace", XS_uwsgi_stacktrace, "uwsgi");


        uperl.tmp_streaming_stash[uperl.tmp_current_i] = gv_stashpv("uwsgi::streaming", 0);

nonworker:

        init_perl_embedded_module();

}

/* end of automagically generated part */

PerlInterpreter *uwsgi_perl_new_interpreter(void) {

	PerlInterpreter *pi = perl_alloc();
        if (!pi) {
                uwsgi_log("unable to allocate perl interpreter\n");
                return NULL;
        }

	PERL_SET_CONTEXT(pi);

        PL_perl_destruct_level = 2;
        PL_origalen = 1;
        perl_construct(pi);
	// over-engeneering
        PL_origalen = 1;

	return pi;
}

static void uwsgi_perl_free_stashes(void) {
        free(uperl.tmp_streaming_stash);
        free(uperl.tmp_input_stash);
        free(uperl.tmp_error_stash);
        free(uperl.tmp_stream_responder);
        free(uperl.tmp_psgix_logger);
}

int init_psgi_app(struct wsgi_request *wsgi_req, char *app, uint16_t app_len, PerlInterpreter **interpreters) {

	int i;
	SV **callables;

	time_t now = uwsgi_now();

	char *app_name = uwsgi_concat2n(app, app_len, "", 0);

	size_t size;
	char *buf = uwsgi_open_and_read(app_name, &size, 1, NULL);

	if (uwsgi_file_exists(app_name)) {
		// prepare for $0 (if the file is local)
		uperl.embedding[1] = app_name;
	}

	// the first (default) app, should always be loaded in the main interpreter
	if (interpreters == NULL) {
		if (uwsgi_apps_cnt) {
			interpreters = uwsgi_calloc(sizeof(PerlInterpreter *) * uwsgi.threads);
			interpreters[0] = uwsgi_perl_new_interpreter();
			if (!interpreters[0]) {
				uwsgi_log("unable to create new perl interpreter\n");
				free(interpreters);
				goto clear2;
			}
		}
		else {
			interpreters = uperl.main;
		}		
	}

	if (!interpreters) goto clear2;

	callables = uwsgi_calloc(sizeof(SV *) * uwsgi.threads);
	uperl.tmp_streaming_stash = uwsgi_calloc(sizeof(HV *) * uwsgi.threads);
	uperl.tmp_input_stash = uwsgi_calloc(sizeof(HV *) * uwsgi.threads);
	uperl.tmp_error_stash = uwsgi_calloc(sizeof(HV *) * uwsgi.threads);
	uperl.tmp_stream_responder = uwsgi_calloc(sizeof(CV *) * uwsgi.threads);
	uperl.tmp_psgix_logger = uwsgi_calloc(sizeof(CV *) * uwsgi.threads);

	for(i=0;i<uwsgi.threads;i++) {

		if (i > 0 && interpreters != uperl.main) {
		
			interpreters[i] = uwsgi_perl_new_interpreter();
			if (!interpreters[i]) {
				uwsgi_log("unable to create new perl interpreter\n");
				// what to do here ? i hope no-one will use threads with dynamic apps...but clear the whole stuff...
				free(callables);
				uwsgi_perl_free_stashes();
				while(i>=0) {
					perl_destruct(interpreters[i]);	
					perl_free(interpreters[i]);
					goto clear2;
				}
			}
		}

		PERL_SET_CONTEXT(interpreters[i]);

		uperl.tmp_current_i = i;


		if (uperl.locallib) {
                        uwsgi_log("using %s as local::lib directory\n", uperl.locallib);
                        uperl.embedding[1] = uwsgi_concat2("-Mlocal::lib=", uperl.locallib);
                        uperl.embedding[2] = app_name;
                        if (perl_parse(interpreters[i], xs_init, 3, uperl.embedding, NULL)) {
				// what to do here ? i hope no-one will use threads with dynamic apps... but clear the whole stuff...
				free(uperl.embedding[1]);
				uperl.embedding[1] = app_name;
				free(callables);
				uwsgi_perl_free_stashes();
				goto clear;
                        }
			free(uperl.embedding[1]);
			uperl.embedding[1] = app_name;
                }
		else {
			if (perl_parse(interpreters[i], xs_init, 2, uperl.embedding, NULL)) {
				// what to do here ? i hope no-one will use threads with dynamic apps... but clear the whole stuff...
				free(callables);
				uwsgi_perl_free_stashes();
				goto clear;
        		}
		}

		perl_eval_pv("use IO::Handle;", 0);
		perl_eval_pv("use IO::File;", 0);
		perl_eval_pv("use IO::Socket;", 0);
		perl_eval_pv("use Scalar::Util;", 0);
		if (!uperl.no_die_catch) {
			perl_eval_pv("use Devel::StackTrace;", 0);
			if (!SvTRUE(ERRSV)) {
				uperl.stacktrace_available = 1;
				perl_eval_pv("$SIG{__DIE__} = \\&uwsgi::stacktrace;", 0);
			}
		}

		if (uperl.argv_items || uperl.argv_item) {
			AV *uperl_argv = GvAV(PL_argvgv);
			if (uperl.argv_items) {
				char *argv_list = uwsgi_str(uperl.argv_items);
				char *p, *ctx = NULL;
				uwsgi_foreach_token(argv_list, " ", p, ctx) {
					av_push(uperl_argv, newSVpv(p, 0));
				}
			}
			struct uwsgi_string_list *usl = uperl.argv_item;
			while(usl) {
				av_push(uperl_argv, newSVpv(usl->value, usl->len));
				usl = usl->next;
			}
		}
		

		SV *dollar_zero = get_sv("0", GV_ADD);
		sv_setsv(dollar_zero, newSVpv(app, app_len));

		callables[i] = perl_eval_pv(uwsgi_concat4("#line 1 ", app_name, "\n", buf), 0);
		if (!callables[i]) {
			uwsgi_log("unable to find PSGI function entry point.\n");
			// what to do here ? i hope no-one will use threads with dynamic apps...
			free(callables);
			uwsgi_perl_free_stashes();
                	goto clear;
		}

		PERL_SET_CONTEXT(interpreters[0]);
	}

	free(buf);

	if(SvTRUE(ERRSV)) {
        	uwsgi_log("%s\n", SvPV_nolen(ERRSV));
		free(callables);
		uwsgi_perl_free_stashes();
		goto clear;
        }

	if (uwsgi_apps_cnt >= uwsgi.max_apps) {
		uwsgi_log("ERROR: you cannot load more than %d apps in a worker\n", uwsgi.max_apps);
		goto clear;
	}

	int id = uwsgi_apps_cnt;
	struct uwsgi_app *wi = NULL;

	if (wsgi_req) {
		// we need a copy of app_id
		wi = uwsgi_add_app(id, psgi_plugin.modifier1, uwsgi_concat2n(wsgi_req->appid, wsgi_req->appid_len, "", 0), wsgi_req->appid_len, interpreters, callables);
	}
	else {
		wi = uwsgi_add_app(id, psgi_plugin.modifier1, "", 0, interpreters, callables);
	}

	wi->started_at = now;
	wi->startup_time = uwsgi_now() - now;

        uwsgi_log("PSGI app %d (%s) loaded in %d seconds at %p (interpreter %p)\n", id, app_name, (int) wi->startup_time, callables[0], interpreters[0]);
	free(app_name);

	// copy global data to app-specific areas
	wi->stream = uperl.tmp_streaming_stash;
	wi->input = uperl.tmp_input_stash;
	wi->error = uperl.tmp_error_stash;
	wi->responder0 = uperl.tmp_stream_responder;
	wi->responder1 = uperl.tmp_psgix_logger;

	uwsgi_emulate_cow_for_apps(id);


	// restore context if required
	if (interpreters != uperl.main) {
		PERL_SET_CONTEXT(uperl.main[0]);
	}

	uperl.loaded = 1;

	return id;

clear:
	if (interpreters != uperl.main) {
		for(i=0;i<uwsgi.threads;i++) {
			perl_destruct(interpreters[i]);
			perl_free(interpreters[i]);
		}
		free(interpreters);
	}

	PERL_SET_CONTEXT(uperl.main[0]);
clear2:
	free(app_name);
       	return -1; 
}

void uwsgi_psgi_app() {

        if (uperl.psgi) {
		//load app in the main interpreter list
		init_psgi_app(NULL, uperl.psgi, strlen(uperl.psgi), uperl.main);
        }
	// create a perl environment (if needed)
	else if (uperl.exec || uperl.shell) {
		PERL_SET_CONTEXT(uperl.main[0]);
                perl_parse(uperl.main[0], xs_init, 2, uperl.embedding, NULL);
	}

	struct uwsgi_string_list *usl;
        uwsgi_foreach(usl, uperl.exec) {
                uwsgi_perl_exec(usl->value);
        }

}

int uwsgi_perl_mule(char *opt) {

        if (uwsgi_endswith(opt, ".pl")) {
                PERL_SET_CONTEXT(uperl.main[0]);
                uperl.embedding[1] = opt;
                if (perl_parse(uperl.main[0], xs_init, 2, uperl.embedding, NULL)) {
                        return 0;
                }
                perl_run(uperl.main[0]);
                return 1;
        }

        return 0;

}


void uwsgi_perl_exec(char *filename) {
	size_t size = 0;
        char *buf = uwsgi_open_and_read(filename, &size, 1, NULL);
        perl_eval_pv(buf, 0);
	free(buf);
}
