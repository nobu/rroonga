/* -*- c-file-style: "ruby" -*- */
/*
  Copyright (C) 2009  Kouhei Sutou <kou@clear-code.com>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License version 2.1 as published by the Free Software Foundation.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "rb-grn.h"

#define SELF(object) ((RbGrnTable *)DATA_PTR(object))

VALUE rb_cGrnTable;

grn_obj *
rb_grn_table_from_ruby_object (VALUE object, grn_ctx **context)
{
    if (!RVAL2CBOOL(rb_obj_is_kind_of(object, rb_cGrnTable))) {
	rb_raise(rb_eTypeError, "not a groonga table");
    }

    return RVAL2GRNOBJECT(object, context);
}

VALUE
rb_grn_table_to_ruby_object (grn_ctx *context, grn_obj *table,
			     rb_grn_boolean owner)
{
    return GRNOBJECT2RVAL(rb_cGrnTable, context, table, owner);
}

void
rb_grn_table_unbind (RbGrnTable *rb_grn_table)
{
    RbGrnObject *rb_grn_object;
    grn_ctx *context;

    rb_grn_object = RB_GRN_OBJECT(rb_grn_table);
    context = rb_grn_object->context;

    if (context)
	grn_obj_close(context, rb_grn_table->value);

    rb_grn_object_unbind(rb_grn_object);
}

static void
rb_grn_table_free (void *object)
{
    RbGrnTable *rb_grn_table = object;

    rb_grn_table_unbind(rb_grn_table);
    xfree(rb_grn_table);
}

VALUE
rb_grn_table_alloc (VALUE klass)
{
    return Data_Wrap_Struct(klass, NULL, rb_grn_table_free, NULL);
}

void
rb_grn_table_bind (RbGrnTable *rb_grn_table,
		   grn_ctx *context, grn_obj *table, rb_grn_boolean owner)
{
    RbGrnObject *rb_grn_object;

    rb_grn_object = RB_GRN_OBJECT(rb_grn_table);
    rb_grn_object_bind(rb_grn_object, context, table, owner);
    rb_grn_object->unbind = RB_GRN_UNBIND_FUNCTION(rb_grn_table_unbind);

    rb_grn_table->value = grn_obj_open(context, GRN_BULK, 0,
				       rb_grn_object->range_id);
}

void
rb_grn_table_assign (VALUE self, VALUE rb_context,
		     grn_ctx *context, grn_obj *table,
		     rb_grn_boolean owner)
{
    RbGrnTable *rb_grn_table;

    rb_grn_table = ALLOC(RbGrnTable);
    DATA_PTR(self) = rb_grn_table;
    rb_grn_table_bind(rb_grn_table, context, table, owner);

    rb_iv_set(self, "context", rb_context);
}

void
rb_grn_table_deconstruct (RbGrnTable *rb_grn_table,
			  grn_obj **table,
			  grn_ctx **context,
			  grn_id *domain_id,
			  grn_obj **domain,
			  grn_obj **value,
			  grn_id *range_id,
			  grn_obj **range)
{
    RbGrnObject *rb_grn_object;

    rb_grn_object = RB_GRN_OBJECT(rb_grn_table);
    rb_grn_object_deconstruct(rb_grn_object, table, context,
			      domain_id, domain,
			      range_id, range);

    if (value)
	*value = rb_grn_table->value;
}

VALUE
rb_grn_table_s_create (int argc, VALUE *argv, VALUE klass,
		       grn_obj_flags key_store)
{
    grn_ctx *context;
    grn_obj *key_type = NULL, *table;
    const char *name = NULL, *path = NULL;
    unsigned name_size = 0, value_size = 0;
    grn_obj_flags flags = key_store;
    VALUE rb_table;
    VALUE options, rb_context, rb_name, rb_path, rb_persistent;
    VALUE rb_key_normalize, rb_key_with_sis, rb_key_type;
    VALUE rb_value_size;

    rb_scan_args(argc, argv, "01", &options);

    rb_grn_scan_options(options,
			"context", &rb_context,
			"name", &rb_name,
                        "path", &rb_path,
			"persistent", &rb_persistent,
			"key_normalize", &rb_key_normalize,
			"key_with_sis", &rb_key_with_sis,
			"key_type", &rb_key_type,
			"value_size", &rb_value_size,
			NULL);

    context = rb_grn_context_ensure(&rb_context);

    if (!NIL_P(rb_name)) {
        name = StringValuePtr(rb_name);
	name_size = RSTRING_LEN(rb_name);
    }

    if (!NIL_P(rb_path)) {
        path = StringValueCStr(rb_path);
	flags |= GRN_OBJ_PERSISTENT;
    }

    if (RVAL2CBOOL(rb_persistent))
	flags |= GRN_OBJ_PERSISTENT;

    if (RVAL2CBOOL(rb_key_normalize))
	flags |= GRN_OBJ_KEY_NORMALIZE;

    if (RVAL2CBOOL(rb_key_with_sis))
	flags |= GRN_OBJ_KEY_WITH_SIS;

    if (NIL_P(rb_key_type)) {
	flags |= GRN_OBJ_KEY_VAR_SIZE;
    } else {
	key_type = RVAL2GRNOBJECT(rb_key_type, &context);
    }

    if (!NIL_P(rb_value_size))
	value_size = NUM2UINT(rb_value_size);

    table = grn_table_create(context, name, name_size, path,
			     flags, key_type, value_size);
    rb_table = rb_grn_table_alloc(klass);
    rb_grn_table_assign(rb_table, rb_context, context, table, RB_GRN_TRUE);
    rb_grn_context_check(context, rb_table);

    if (rb_block_given_p())
        return rb_ensure(rb_yield, rb_table, rb_grn_object_close, rb_table);
    else
        return rb_table;
}

grn_obj *
rb_grn_table_open_raw (int argc, VALUE *argv,
		       grn_ctx **context, VALUE *rb_context)
{
    grn_obj *table;
    char *name = NULL, *path = NULL;
    unsigned name_size = 0;
    VALUE rb_path, options, rb_name;

    rb_scan_args(argc, argv, "01", &options);

    rb_grn_scan_options(options,
			"context", rb_context,
			"name", &rb_name,
			"path", &rb_path,
			NULL);

    *context = rb_grn_context_ensure(rb_context);

    if (!NIL_P(rb_name)) {
	name = StringValuePtr(rb_name);
	name_size = RSTRING_LEN(rb_name);
    }

    if (!NIL_P(rb_path))
	path = StringValueCStr(rb_path);

    table = grn_table_open(*context, name, name_size, path);
    return table;
}

static VALUE
rb_grn_table_initialize (int argc, VALUE *argv, VALUE self)
{
    grn_ctx *context = NULL;
    grn_obj *table;
    VALUE rb_context;

    table = rb_grn_table_open_raw(argc, argv, &context, &rb_context);
    rb_grn_object_assign(self, rb_context, context, table, RB_GRN_TRUE);
    rb_grn_context_check(context, self);

    return Qnil;
}

static VALUE
rb_grn_table_s_open (int argc, VALUE *argv, VALUE klass)
{
    VALUE rb_table;
    grn_obj *table;
    grn_ctx *context = NULL;
    VALUE rb_context;

    table = rb_grn_table_open_raw(argc, argv, &context, &rb_context);
    rb_grn_context_check(context, rb_ary_new4(argc, argv));

    if (!table)
	rb_raise(rb_eGrnError,
		 "unable to open table: %s: %s",
		 rb_grn_inspect(klass),
		 rb_grn_inspect(rb_ary_new4(argc, argv)));

    if (klass == rb_cGrnTable) {
	klass = GRNOBJECT2RCLASS(table);
    } else {
	VALUE rb_class;

	rb_class = GRNOBJECT2RCLASS(table);
	if (rb_class != klass) {
	    rb_raise(rb_eTypeError,
		     "unexpected existing table type: %s: expected %s",
		     rb_grn_inspect(rb_class),
		     rb_grn_inspect(klass));
	}
    }

    rb_table = rb_grn_object_alloc(klass);
    rb_grn_object_assign(rb_table, rb_context, context, table, RB_GRN_TRUE);

    if (rb_block_given_p())
        return rb_ensure(rb_yield, rb_table, rb_grn_object_close, rb_table);
    else
        return rb_table;
}

static VALUE
rb_grn_table_inspect_content (VALUE self, VALUE inspected)
{
    grn_ctx *context = NULL;
    grn_obj *table;

    rb_grn_table_deconstruct(SELF(self), &table, &context,
			     NULL, NULL,
			     NULL, NULL, NULL);

    if (!table)
	return inspected;
    if (!context)
	return inspected;

    if (table->header.type != GRN_TABLE_NO_KEY) {
	grn_obj value;
	grn_encoding encoding;

	rb_str_cat2(inspected, ", ");
	rb_str_cat2(inspected, "encoding: <");
	GRN_OBJ_INIT(&value, GRN_BULK, 0, GRN_ID_NIL);
	grn_obj_get_info(context, table, GRN_INFO_ENCODING, &value);
	encoding = *((grn_encoding *)GRN_BULK_HEAD(&value));
	grn_obj_close(context, &value);

	if (context->rc == GRN_SUCCESS)
	    rb_str_concat(inspected, rb_inspect(GRNENCODING2RVAL(encoding)));
	else
	    rb_str_cat2(inspected, "invalid");

	rb_str_cat2(inspected, ">");
    }

    rb_str_cat2(inspected, ", ");
    rb_str_cat2(inspected, "size: <");
    {
	char buf[21]; /* ceil(log10(2 ** 64)) + 1('\0') == 21 */
	snprintf(buf, sizeof(buf), "%u", grn_table_size(context, table));
	rb_str_cat2(inspected, buf);
    }
    rb_str_cat2(inspected, ">");

    return inspected;
}

static VALUE
rb_grn_table_inspect (VALUE self)
{
    VALUE inspected;

    inspected = rb_str_new2("");
    rb_grn_object_inspect_header(self, inspected);
    rb_grn_object_inspect_content(self, inspected);
    rb_grn_table_inspect_content(self, inspected);
    rb_grn_object_inspect_footer(self, inspected);

    return inspected;
}

static VALUE
rb_grn_table_define_column (int argc, VALUE *argv, VALUE self)
{
    grn_ctx *context = NULL;
    grn_obj *table;
    grn_obj *value_type, *column;
    char *name = NULL, *path = NULL;
    unsigned name_size = 0;
    grn_obj_flags flags = 0;
    VALUE rb_name, rb_value_type;
    VALUE options, rb_path, rb_persistent, rb_type;

    rb_grn_table_deconstruct(SELF(self), &table, &context,
			     NULL, NULL,
			     NULL, NULL, NULL);

    rb_scan_args(argc, argv, "21", &rb_name, &rb_value_type, &options);

    name = StringValuePtr(rb_name);
    name_size = RSTRING_LEN(rb_name);

    rb_grn_scan_options(options,
			"path", &rb_path,
			"persistent", &rb_persistent,
			"type", &rb_type,
			NULL);

    value_type = RVAL2GRNOBJECT(rb_value_type, &context);

    if (!NIL_P(rb_path)) {
	path = StringValueCStr(rb_path);
	flags |= GRN_OBJ_PERSISTENT;
    }

    if (RVAL2CBOOL(rb_persistent))
	flags |= GRN_OBJ_PERSISTENT;

    if (NIL_P(rb_type) ||
	(rb_grn_equal_option(rb_type, "scalar"))) {
	flags |= GRN_OBJ_COLUMN_SCALAR;
    } else if (rb_grn_equal_option(rb_type, "vector")) {
	flags |= GRN_OBJ_COLUMN_VECTOR;
    } else {
	rb_raise(rb_eArgError,
		 "invalid column type: %s: "
		 "available types: [:scalar, :vector, nil]",
		 rb_grn_inspect(rb_type));
    }

    column = grn_column_create(context, table, name, name_size,
			       path, flags, value_type);
    rb_grn_context_check(context, self);

    return GRNCOLUMN2RVAL(Qnil, context, column, RB_GRN_TRUE);
}

static VALUE
rb_grn_table_define_index_column (int argc, VALUE *argv, VALUE self)
{
    grn_ctx *context = NULL;
    grn_obj *table;
    grn_obj *value_type, *column;
    char *name = NULL, *path = NULL;
    unsigned name_size = 0;
    grn_obj_flags flags = GRN_OBJ_COLUMN_INDEX;
    VALUE rb_name, rb_value_type;
    VALUE options, rb_path, rb_persistent;
    VALUE rb_compress, rb_with_section, rb_with_weight, rb_with_position;
    VALUE rb_column, rb_source, rb_sources;

    rb_grn_table_deconstruct(SELF(self), &table, &context,
			     NULL, NULL,
			     NULL, NULL, NULL);

    rb_scan_args(argc, argv, "21", &rb_name, &rb_value_type, &options);

    name = StringValuePtr(rb_name);
    name_size = RSTRING_LEN(rb_name);

    rb_grn_scan_options(options,
			"path", &rb_path,
			"persistent", &rb_persistent,
			"compress", &rb_compress,
			"with_section", &rb_with_section,
			"with_weight", &rb_with_weight,
			"with_position", &rb_with_position,
			"source", &rb_source,
			"sources", &rb_sources,
			NULL);

    value_type = RVAL2GRNOBJECT(rb_value_type, &context);

    if (!NIL_P(rb_path)) {
	path = StringValueCStr(rb_path);
	flags |= GRN_OBJ_PERSISTENT;
    }

    if (RVAL2CBOOL(rb_persistent))
	flags |= GRN_OBJ_PERSISTENT;

    if (NIL_P(rb_compress)) {
    } else if (rb_grn_equal_option(rb_compress, "zlib")) {
	flags |= GRN_OBJ_COMPRESS_ZLIB;
    } else if (rb_grn_equal_option(rb_compress, "lzo")) {
	flags |= GRN_OBJ_COMPRESS_LZO;
    } else {
	rb_raise(rb_eArgError,
		 "invalid compress type: %s: "
		 "available types: [:zlib, :lzo, nil]",
		 rb_grn_inspect(rb_compress));
    }

    if (RVAL2CBOOL(rb_with_section))
	flags |= GRN_OBJ_WITH_SECTION;

    if (RVAL2CBOOL(rb_with_weight))
	flags |= GRN_OBJ_WITH_WEIGHT;

    if (RVAL2CBOOL(rb_with_position))
	flags |= GRN_OBJ_WITH_POSITION;

    if (!NIL_P(rb_source) && !NIL_P(rb_sources))
	rb_raise(rb_eArgError, "should not pass both of :source and :sources.");

    column = grn_column_create(context, table, name, name_size,
			       path, flags, value_type);
    rb_grn_context_check(context, self);

    rb_column = GRNCOLUMN2RVAL(Qnil, context, column, RB_GRN_TRUE);
    if (!NIL_P(rb_source))
	rb_funcall(rb_column, rb_intern("source="), 1, rb_source);
    if (!NIL_P(rb_sources))
	rb_funcall(rb_column, rb_intern("sources="), 1, rb_sources);

    return rb_column;
}

static VALUE
rb_grn_table_add_column (VALUE self, VALUE rb_name, VALUE rb_value_type,
			 VALUE rb_path)
{
    grn_ctx *context = NULL;
    grn_obj *table;
    grn_obj *value_type, *column;
    char *name = NULL, *path = NULL;
    unsigned name_size = 0;
    VALUE rb_column;

    rb_grn_table_deconstruct(SELF(self), &table, &context,
			     NULL, NULL,
			     NULL, NULL, NULL);

    name = StringValuePtr(rb_name);
    name_size = RSTRING_LEN(rb_name);

    value_type = RVAL2GRNOBJECT(rb_value_type, &context);

    path = StringValueCStr(rb_path);

    column = grn_column_open(context, table, name, name_size,
			     path, value_type);
    rb_grn_context_check(context, self);

    rb_column = GRNCOLUMN2RVAL(Qnil, context, column, RB_GRN_TRUE);
    rb_iv_set(rb_column, "table", self);
    return rb_column;
}

static VALUE
rb_grn_table_get_column (VALUE self, VALUE rb_name)
{
    grn_ctx *context = NULL;
    grn_obj *table;
    grn_obj *column;
    char *name = NULL;
    unsigned name_size = 0;
    rb_grn_boolean owner;
    VALUE rb_column;

    rb_grn_table_deconstruct(SELF(self), &table, &context,
			     NULL, NULL,
			     NULL, NULL, NULL);

    name = StringValuePtr(rb_name);
    name_size = RSTRING_LEN(rb_name);

    column = grn_obj_column(context, table, name, name_size);
    rb_grn_context_check(context, self);

    owner = (column && column->header.type == GRN_ACCESSOR);
    rb_column = GRNCOLUMN2RVAL(Qnil, context, column, owner);
    if (owner)
	rb_iv_set(rb_column, "table", self);
    return rb_column;
}

static VALUE
rb_grn_table_get_columns (int argc, VALUE *argv, VALUE self)
{
    grn_ctx *context = NULL;
    grn_obj *table;
    grn_obj *columns;
    grn_rc rc;
    int n;
    grn_table_cursor *cursor;
    VALUE rb_name, rb_columns;
    char *name = NULL;
    unsigned name_size = 0;

    rb_grn_table_deconstruct(SELF(self), &table, &context,
			     NULL, NULL,
			     NULL, NULL, NULL);

    rb_scan_args(argc, argv, "01", &rb_name);

    if (!NIL_P(rb_name)) {
	name = StringValuePtr(rb_name);
	name_size = RSTRING_LEN(rb_name);
    }

    columns = grn_table_create(context, NULL, 0, NULL, GRN_TABLE_HASH_KEY,
			       NULL, 0);
    n = grn_table_columns(context, table, name, name_size, columns);
    rb_grn_context_check(context, self);

    rb_columns = rb_ary_new2(n);
    if (n == 0)
	return rb_columns;

    cursor = grn_table_cursor_open(context, columns, NULL, 0, NULL, 0,
				   GRN_CURSOR_ASCENDING);
    rb_grn_context_check(context, self);
    while (grn_table_cursor_next(context, cursor) != GRN_ID_NIL) {
	void *key;
	grn_id *column_id;
	grn_obj *column;
	VALUE rb_column;

	grn_table_cursor_get_key(context, cursor, &key);
	column_id = key;
	column = grn_ctx_at(context, *column_id);
	rb_column = GRNOBJECT2RVAL(Qnil, context, column, RB_GRN_FALSE);
	rb_ary_push(rb_columns, rb_column);
    }
    rc = grn_table_cursor_close(context, cursor);
    if (rc != GRN_SUCCESS) {
	rb_grn_context_check(context, self);
	rb_grn_rc_check(rc, self);
    }

    return rb_columns;
}

static grn_table_cursor *
rb_grn_table_open_grn_cursor (int argc, VALUE *argv, VALUE self,
			      grn_ctx **context)
{
    grn_obj *table;
    grn_table_cursor *cursor;
    void *min_key = NULL, *max_key = NULL;
    unsigned min_key_size = 0, max_key_size = 0;
    int flags = 0;
    VALUE options, rb_min, rb_max, rb_order, rb_greater_than, rb_less_than;

    rb_grn_table_deconstruct(SELF(self), &table, context,
			     NULL, NULL,
			     NULL, NULL, NULL);

    rb_scan_args(argc, argv, "01", &options);

    rb_grn_scan_options(options,
			"min", &rb_min,
                        "max", &rb_max,
			"order", &rb_order,
			"greater_than", &rb_greater_than,
			"less_than", &rb_less_than,
			NULL);

    if (!NIL_P(rb_min)) {
	min_key = StringValuePtr(rb_min);
	min_key_size = RSTRING_LEN(rb_min);
    }
    if (!NIL_P(rb_max)) {
	max_key = StringValuePtr(rb_max);
	max_key_size = RSTRING_LEN(rb_max);
    }

    if (NIL_P(rb_order)) {
    } else if (rb_grn_equal_option(rb_order, "asc") ||
	       rb_grn_equal_option(rb_order, "ascending")) {
	flags |= GRN_CURSOR_ASCENDING;
    } else if (rb_grn_equal_option(rb_order, "desc") ||
	       rb_grn_equal_option(rb_order, "descending")) {
	flags |= GRN_CURSOR_DESCENDING;
    } else {
	rb_raise(rb_eArgError,
		 "order should be one of "
		 "[:asc, :ascending, :desc, :descending]: %s",
		 rb_grn_inspect(rb_order));
    }

    if (RVAL2CBOOL(rb_greater_than))
	flags |= GRN_CURSOR_GT;
    if (RVAL2CBOOL(rb_less_than))
	flags |= GRN_CURSOR_LT;

    cursor = grn_table_cursor_open(*context, table,
				   min_key, min_key_size,
				   max_key, max_key_size,
				   flags);
    rb_grn_context_check(*context, self);

    return cursor;
}

static VALUE
rb_grn_table_open_cursor (int argc, VALUE *argv, VALUE self)
{
    grn_ctx *context = NULL;
    grn_table_cursor *cursor;
    VALUE rb_cursor;

    cursor = rb_grn_table_open_grn_cursor(argc, argv, self, &context);
    rb_cursor = GRNTABLECURSOR2RVAL(Qnil, context, cursor);
    rb_iv_set(rb_cursor, "@table", self); /* FIXME: cursor should mark table */
    if (rb_block_given_p())
	return rb_ensure(rb_yield, rb_cursor,
			 rb_grn_table_cursor_close, rb_cursor);
    else
	return rb_cursor;
}

static VALUE
rb_grn_table_get_records (int argc, VALUE *argv, VALUE self)
{
    grn_ctx *context = NULL;
    grn_table_cursor *cursor;
    grn_id record_id;
    VALUE records;

    cursor = rb_grn_table_open_grn_cursor(argc, argv, self, &context);
    records = rb_ary_new();
    while ((record_id = grn_table_cursor_next(context, cursor))) {
	rb_ary_push(records, rb_grn_record_new(self, record_id, Qnil));
    }
    grn_table_cursor_close(context, cursor);

    return records;
}

static VALUE
rb_grn_table_get_size (VALUE self)
{
    grn_ctx *context = NULL;
    grn_obj *table;
    unsigned int size;

    rb_grn_table_deconstruct(SELF(self), &table, &context,
			     NULL, NULL,
			     NULL, NULL, NULL);
    size = grn_table_size(context, table);
    return UINT2NUM(size);
}

static VALUE
rb_grn_table_truncate (VALUE self)
{
    grn_ctx *context = NULL;
    grn_obj *table;
    grn_rc rc;

    rb_grn_table_deconstruct(SELF(self), &table, &context,
			     NULL, NULL,
			     NULL, NULL, NULL);
    rc = grn_table_truncate(context, table);
    rb_grn_rc_check(rc, self);

    return Qnil;
}

static VALUE
rb_grn_table_each (VALUE self)
{
    grn_ctx *context = NULL;
    grn_obj *table;
    grn_table_cursor *cursor;
    VALUE rb_cursor;
    grn_id id;

    rb_grn_table_deconstruct(SELF(self), &table, &context,
			     NULL, NULL,
			     NULL, NULL, NULL);
    cursor = grn_table_cursor_open(context, table, NULL, 0, NULL, 0, 0);
    rb_cursor = GRNTABLECURSOR2RVAL(Qnil, context, cursor);
    rb_iv_set(self, "cursor", rb_cursor);
    while ((id = grn_table_cursor_next(context, cursor)) != GRN_ID_NIL) {
	rb_yield(rb_grn_record_new(self, id, Qnil));
    }
    rb_grn_table_cursor_close(rb_cursor);
    rb_iv_set(self, "cursor", Qnil);

    return Qnil;
}

VALUE
rb_grn_table_delete (VALUE self, VALUE rb_id)
{
    grn_ctx *context = NULL;
    grn_obj *table;
    grn_id id;
    grn_rc rc;

    rb_grn_table_deconstruct(SELF(self), &table, &context,
			     NULL, NULL,
			     NULL, NULL, NULL);

    id = NUM2UINT(rb_id);
    rc = grn_table_delete_by_id(context, table, id);
    rb_grn_rc_check(rc, self);

    return Qnil;
}

static VALUE
rb_grn_table_sort (int argc, VALUE *argv, VALUE self)
{
    grn_ctx *context = NULL;
    grn_obj *table;
    grn_obj *result;
    grn_table_sort_key *keys;
    int n_records, limit = 0;
    int i, n_keys;
    VALUE rb_keys, options;
    VALUE rb_limit;
    VALUE *rb_sort_keys;

    rb_grn_table_deconstruct(SELF(self), &table, &context,
			     NULL, NULL,
			     NULL, NULL, NULL);

    rb_scan_args(argc, argv, "11", &rb_keys, &options);
    rb_grn_scan_options(options,
			"limit", &rb_limit,
			NULL);

    n_keys = RARRAY_LEN(rb_keys);
    rb_sort_keys = RARRAY_PTR(rb_keys);
    keys = ALLOCA_N(grn_table_sort_key, n_keys);
    for (i = 0; i < n_keys; i++) {
	VALUE rb_sort_options, rb_key, rb_order;

	if (RVAL2CBOOL(rb_obj_is_kind_of(rb_sort_keys[i], rb_cHash))) {
	    rb_sort_options = rb_sort_keys[i];
	} else {
	    rb_sort_options = rb_hash_new();
	    rb_hash_aset(rb_sort_options,
			 RB_GRN_INTERN("key"),
			 rb_sort_keys[i]);
	}
	rb_grn_scan_options(rb_sort_options,
			    "key", &rb_key,
			    "order", &rb_order,
			    NULL);
	if (RVAL2CBOOL(rb_obj_is_kind_of(rb_key, rb_cString)))
	    rb_key = rb_grn_table_get_column(self, rb_key);
	keys[i].key = RVAL2GRNOBJECT(rb_key, &context);
	if (NIL_P(rb_order)) {
	    keys[i].flags = 0;
	} else if (rb_grn_equal_option(rb_order, "desc") ||
		   rb_grn_equal_option(rb_order, "descending")) {
	    keys[i].flags = GRN_TABLE_SORT_DESC;
	} else {
	    /* FIXME: validation */
	    keys[i].flags = GRN_TABLE_SORT_ASC;
	}
    }

    if (!NIL_P(rb_limit))
	limit = NUM2INT(rb_limit);

    result = grn_table_create(context, NULL, 0, NULL, GRN_TABLE_NO_KEY,
			      table, sizeof(grn_id));
    n_records = grn_table_sort(context, table, limit,
			       result, keys, n_keys);

    return GRNOBJECT2RVAL(Qnil, context, result, RB_GRN_TRUE);
}

/*
 * Document-method: []
 *
 * call-seq:
 *   table[id] -> 値
 *
 * _table_の_id_に対応する値を返す。
 */
VALUE
rb_grn_table_array_reference (VALUE self, VALUE rb_id)
{
    grn_id id;
    grn_ctx *context;
    grn_obj *table;
    grn_obj *range;
    grn_obj *value;

    rb_grn_table_deconstruct(SELF(self), &table, &context,
			     NULL, NULL,
			     &value, NULL, &range);

    id = NUM2UINT(rb_id);
    GRN_BULK_REWIND(value);
    grn_obj_get_value(context, table, id, value);
    rb_grn_context_check(context, self);

    if (GRN_BULK_EMPTYP(value))
	return Qnil;
    else
	return rb_str_new(GRN_BULK_HEAD(value), GRN_BULK_VSIZE(value));
}

/*
 * Document-method: []=
 *
 * call-seq:
 *   table[id] = value
 *
 * _table_の_id_に対応する値を設定する。既存の値は上書きさ
 * れる。
 */
VALUE
rb_grn_table_array_set (VALUE self, VALUE rb_id, VALUE rb_value)
{
    grn_id id;
    grn_ctx *context;
    grn_obj *table;
    grn_obj *range;
    grn_obj *value;
    grn_rc rc;

    rb_grn_table_deconstruct(SELF(self), &table, &context,
			     NULL, NULL,
			     &value, NULL, &range);

    id = NUM2UINT(rb_id);
    GRN_BULK_REWIND(value);
    RVAL2GRNBULK(rb_value, context, value);
    rc = grn_obj_set_value(context, table, id, value, GRN_OBJ_SET);
    rb_grn_context_check(context, self);
    rb_grn_rc_check(rc, self);

    return Qnil;
}

void
rb_grn_init_table (VALUE mGrn)
{
    rb_cGrnTable = rb_define_class_under(mGrn, "Table", rb_cGrnObject);
    rb_define_alloc_func(rb_cGrnTable, rb_grn_table_alloc);

    rb_include_module(rb_cGrnTable, rb_mEnumerable);

    rb_define_singleton_method(rb_cGrnTable, "open",
			       rb_grn_table_s_open, -1);

    rb_define_method(rb_cGrnTable, "initialize", rb_grn_table_initialize, -1);

    rb_define_method(rb_cGrnTable, "inspect", rb_grn_table_inspect, 0);

    rb_define_method(rb_cGrnTable, "define_column",
		     rb_grn_table_define_column, -1);
    rb_define_method(rb_cGrnTable, "define_index_column",
		     rb_grn_table_define_index_column, -1);
    rb_define_method(rb_cGrnTable, "add_column",
		     rb_grn_table_add_column, 3);
    rb_define_method(rb_cGrnTable, "column",
		     rb_grn_table_get_column, 1);
    rb_define_method(rb_cGrnTable, "columns",
		     rb_grn_table_get_columns, -1);

    rb_define_method(rb_cGrnTable, "open_cursor", rb_grn_table_open_cursor, -1);
    rb_define_method(rb_cGrnTable, "records", rb_grn_table_get_records, -1);

    rb_define_method(rb_cGrnTable, "size", rb_grn_table_get_size, 0);
    rb_define_method(rb_cGrnTable, "truncate", rb_grn_table_truncate, 0);

    rb_define_method(rb_cGrnTable, "each", rb_grn_table_each, 0);

    rb_define_method(rb_cGrnTable, "delete", rb_grn_table_delete, 1);

    rb_define_method(rb_cGrnTable, "sort", rb_grn_table_sort, -1);

    rb_define_method(rb_cGrnTable, "[]", rb_grn_table_array_reference, 1);
    rb_define_method(rb_cGrnTable, "[]=", rb_grn_table_array_set, 2);

    rb_grn_init_table_key_support(mGrn);
    rb_grn_init_array(mGrn);
    rb_grn_init_hash(mGrn);
    rb_grn_init_patricia_trie(mGrn);
}