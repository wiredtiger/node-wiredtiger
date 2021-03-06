/* ----------------------------------------------------------------------------
 * This file was automatically generated by SWIG (http://www.swig.org).
 * Version 2.0.10
 *
 * Do not make changes to this file unless you know what you are doing--modify
 * the SWIG interface file instead.
 * ----------------------------------------------------------------------------- */

package com.wiredtiger.db;

public class wiredtigerJNI {

  static {
    try {
	System.loadLibrary("wiredtiger_java");
    } catch (UnsatisfiedLinkError e) {
      System.err.println("Native code library failed to load. \n" + e);
      System.exit(1);
    }
  }

  public final static native long Cursor_session_get(long jarg1, Cursor jarg1_);
  public final static native String Cursor_uri_get(long jarg1, Cursor jarg1_);
  public final static native String Cursor_key_format_get(long jarg1, Cursor jarg1_);
  public final static native String Cursor_value_format_get(long jarg1, Cursor jarg1_);
  public final static native int Cursor_next_wrap(long jarg1, Cursor jarg1_);
  public final static native int Cursor_prev_wrap(long jarg1, Cursor jarg1_);
  public final static native int Cursor_reset(long jarg1, Cursor jarg1_);
  public final static native int Cursor_close(long jarg1, Cursor jarg1_);
  public final static native byte[] Cursor_get_key_wrap(long jarg1, Cursor jarg1_);
  public final static native byte[] Cursor_get_value_wrap(long jarg1, Cursor jarg1_);
  public final static native int Cursor_insert_wrap(long jarg1, Cursor jarg1_, byte[] jarg2, byte[] jarg3);
  public final static native int Cursor_remove_wrap(long jarg1, Cursor jarg1_, byte[] jarg2);
  public final static native int Cursor_search_wrap(long jarg1, Cursor jarg1_, byte[] jarg2);
  public final static native int Cursor_search_near_wrap(long jarg1, Cursor jarg1_, byte[] jarg3);
  public final static native int Cursor_update_wrap(long jarg1, Cursor jarg1_, byte[] jarg2, byte[] jarg3);
  public final static native int Cursor_compare_wrap(long jarg1, Cursor jarg1_, long jarg3, Cursor jarg3_);
  public final static native int Cursor_java_init(long jarg1, Cursor jarg1_, Object jarg2);
  public final static native long Session_connection_get(long jarg1, Session jarg1_);
  public final static native int Session_close(long jarg1, Session jarg1_, String jarg3);
  public final static native int Session_reconfigure(long jarg1, Session jarg1_, String jarg3);
  public final static native int Session_create(long jarg1, Session jarg1_, String jarg3, String jarg4);
  public final static native int Session_compact(long jarg1, Session jarg1_, String jarg3, String jarg4);
  public final static native int Session_drop(long jarg1, Session jarg1_, String jarg3, String jarg4);
  public final static native int Session_log_printf(long jarg1, Session jarg1_, String jarg3);
  public final static native int Session_rename(long jarg1, Session jarg1_, String jarg3, String jarg4, String jarg5);
  public final static native int Session_salvage(long jarg1, Session jarg1_, String jarg3, String jarg4);
  public final static native int Session_truncate(long jarg1, Session jarg1_, String jarg3, long jarg4, Cursor jarg4_, long jarg5, Cursor jarg5_, String jarg6);
  public final static native int Session_upgrade(long jarg1, Session jarg1_, String jarg3, String jarg4);
  public final static native int Session_verify(long jarg1, Session jarg1_, String jarg3, String jarg4);
  public final static native int Session_begin_transaction(long jarg1, Session jarg1_, String jarg3);
  public final static native int Session_commit_transaction(long jarg1, Session jarg1_, String jarg3);
  public final static native int Session_rollback_transaction(long jarg1, Session jarg1_, String jarg3);
  public final static native int Session_checkpoint(long jarg1, Session jarg1_, String jarg3);
  public final static native int Session_java_init(long jarg1, Session jarg1_, Object jarg2);
  public final static native long Session_open_cursor(long jarg1, Session jarg1_, String jarg3, long jarg4, Cursor jarg4_, String jarg5);
  public final static native int Connection_close(long jarg1, Connection jarg1_, String jarg3);
  public final static native int Connection_reconfigure(long jarg1, Connection jarg1_, String jarg3);
  public final static native String Connection_get_home(long jarg1, Connection jarg1_);
  public final static native int Connection_configure_method(long jarg1, Connection jarg1_, String jarg3, String jarg4, String jarg5, String jarg6, String jarg7);
  public final static native int Connection_is_new(long jarg1, Connection jarg1_);
  public final static native int Connection_load_extension(long jarg1, Connection jarg1_, String jarg3, String jarg4);
  public final static native int Connection_java_init(long jarg1, Connection jarg1_, Object jarg2);
  public final static native long Connection_open_session(long jarg1, Connection jarg1_, String jarg3);
  public final static native String wiredtiger_strerror(int jarg1);
  public final static native long open(String jarg2, String jarg3);
}
