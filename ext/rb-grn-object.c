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

/*
 * Document-class: Groonga::Object
 *
 * Ruby/groongaが提供するクラスのベースとなるクラス。
 * Groonga::ContextとGroonga::Logger以外はGroonga::Objectを継
 * 承している。
 */

#include "rb-grn.h"

#define SELF(object) ((RbGrnObject *)DATA_PTR(object))

VALUE rb_cGrnObject;

grn_obj *
rb_grn_object_from_ruby_object (VALUE object, grn_ctx **context)
{
    RbGrnObject *rb_grn_object;

    if (NIL_P(object))
        return NULL;

    if (context && *context) {
	grn_obj *grn_object;
	if (RVAL2CBOOL(rb_obj_is_kind_of(object, rb_cString))) {
	    grn_object = grn_ctx_get(*context,
                                     StringValuePtr(object),
                                     RSTRING_LEN(object));
	    if (!grn_object)
		rb_raise(rb_eArgError,
			 "unregistered groonga object: name: <%s>",
			 rb_grn_inspect(object));
	    return grn_object;
	} else if (RVAL2CBOOL(rb_obj_is_kind_of(object, rb_cInteger))) {
	    grn_object = grn_ctx_at(*context, NUM2UINT(object));
	    if (!grn_object)
		rb_raise(rb_eArgError,
			 "unregistered groonga object: ID: <%s>",
			 rb_grn_inspect(object));
	    return grn_object;
	}
    }

    if (!RVAL2CBOOL(rb_obj_is_kind_of(object, rb_cGrnObject))) {
	rb_raise(rb_eTypeError, "not a groonga object");
    }

    Data_Get_Struct(object, RbGrnObject, rb_grn_object);
    if (!rb_grn_object)
	rb_raise(rb_eGrnError, "groonga object is NULL");

    if (context && !*context)
	*context = rb_grn_object->context;

    return rb_grn_object->object;
}

void
rb_grn_object_unbind (RbGrnObject *rb_grn_object)
{
    grn_ctx *context;
    grn_obj *grn_object;

    context = rb_grn_object->context;
    grn_object = rb_grn_object->object;

    if (context)
	rb_grn_context_unregister(context, rb_grn_object);

    if (rb_grn_object->owner && context && grn_object) {
	const char *path;
	grn_obj *db = NULL;

	path = grn_obj_path(context, grn_object);
	db = grn_ctx_db(context);
	if (path == NULL || (path && db)) {
	    if (grn_object == db) {
		rb_grn_context_unbind(context);
	    } else {
		grn_obj_close(context, grn_object);
	    }
	}
    }

    rb_grn_object->context = NULL;
    rb_grn_object->object = NULL;
    rb_grn_object->owner = RB_GRN_FALSE;
}

static void
rb_grn_object_free (void *object)
{
    RbGrnObject *rb_grn_object = object;

    rb_grn_object->unbind(rb_grn_object);
    xfree(rb_grn_object);
}

VALUE
rb_grn_object_to_ruby_class (grn_obj *object)
{
    VALUE klass = Qnil;

    switch (object->header.type) {
      case GRN_DB:
	klass = rb_cGrnDatabase;
	break;
      case GRN_TABLE_HASH_KEY:
	klass = rb_cGrnHash;
	break;
      case GRN_TABLE_PAT_KEY:
	klass = rb_cGrnPatriciaTrie;
	break;
      case GRN_TABLE_NO_KEY:
	klass = rb_cGrnArray;
	break;
      case GRN_TYPE:
	klass = rb_cGrnType;
	break;
      case GRN_ACCESSOR:
	klass = rb_cGrnAccessor;
	break;
      case GRN_PROC:
	klass = rb_cGrnProcedure;
	break;
      case GRN_COLUMN_FIX_SIZE:
	klass = rb_cGrnFixSizeColumn;
	break;
      case GRN_COLUMN_VAR_SIZE:
	klass = rb_cGrnVarSizeColumn;
	break;
      case GRN_COLUMN_INDEX:
	klass = rb_cGrnIndexColumn;
	break;
      default:
	rb_raise(rb_eTypeError,
		 "unsupported groonga object type: %d",
		 object->header.type);
	break;
    }

    return klass;
}

VALUE
rb_grn_object_to_ruby_object (VALUE klass, grn_ctx *context, grn_obj *object,
			      rb_grn_boolean owner)
{
    VALUE rb_object;

    if (!object)
        return Qnil;

    if (NIL_P(klass))
        klass = GRNOBJECT2RCLASS(object);

    if (klass == rb_cGrnHash ||
	klass == rb_cGrnPatriciaTrie) {
	rb_object = rb_grn_table_key_support_alloc(klass);
	rb_grn_table_key_support_assign(rb_object, Qnil, context, object, owner);
    } else if (klass == rb_cGrnArray) {
	rb_object = rb_grn_table_alloc(klass);
	rb_grn_table_assign(rb_object, Qnil, context, object, owner);
    } else if (klass == rb_cGrnFixSizeColumn) {
	rb_object = rb_grn_fix_size_column_alloc(klass);
	rb_grn_fix_size_column_assign(rb_object, Qnil, context, object, owner);
    } else if (klass == rb_cGrnIndexColumn) {
	rb_object = rb_grn_index_column_alloc(klass);
	rb_grn_index_column_assign(rb_object, Qnil, context, object, owner);
    } else {
	rb_object = rb_grn_object_alloc(klass);
	rb_grn_object_assign(rb_object, Qnil, context, object, owner);
    }

    return rb_object;
}

VALUE
rb_grn_object_alloc (VALUE klass)
{
    return Data_Wrap_Struct(klass, NULL, rb_grn_object_free, NULL);
}

void
rb_grn_object_bind (RbGrnObject *rb_grn_object,
		    grn_ctx *context, grn_obj *object, rb_grn_boolean owner)
{
    rb_grn_object->context = context;
    rb_grn_object->object = object;

    rb_grn_object->domain_id = GRN_ID_NIL;
    if (object)
	rb_grn_object->domain_id = object->header.domain;
    if (rb_grn_object->domain_id == GRN_ID_NIL)
	rb_grn_object->domain = NULL;
    else
	rb_grn_object->domain = grn_ctx_at(context, rb_grn_object->domain_id);

    rb_grn_object->range_id = GRN_ID_NIL;
    if (object && object->header.type != GRN_TYPE)
	rb_grn_object->range_id = grn_obj_get_range(context, object);
    if (rb_grn_object->range_id == GRN_ID_NIL)
	rb_grn_object->range = NULL;
    else
	rb_grn_object->range = grn_ctx_at(context, rb_grn_object->range_id);

    rb_grn_object->owner = owner;

    rb_grn_object->unbind = RB_GRN_UNBIND_FUNCTION(rb_grn_object_unbind);

    if (context)
	rb_grn_context_register(context, rb_grn_object);
}

void
rb_grn_object_assign (VALUE self, VALUE rb_context,
		      grn_ctx *context, grn_obj *object,
		      rb_grn_boolean owner)
{
    RbGrnObject *rb_grn_object;

    rb_grn_object = ALLOC(RbGrnObject);
    DATA_PTR(self) = rb_grn_object;
    rb_grn_object_bind(rb_grn_object, context, object, owner);

    rb_iv_set(self, "context", rb_context);
}

void
rb_grn_object_deconstruct (RbGrnObject *rb_grn_object,
			   grn_obj **object,
			   grn_ctx **context,
			   grn_id *domain_id,
			   grn_obj **domain,
			   grn_id *range_id,
			   grn_obj **range)
{
    if (!rb_grn_object)
	return;

    if (object)
	*object = rb_grn_object->object;
    if (context)
	*context = rb_grn_object->context;
    if (domain_id)
	*domain_id = rb_grn_object->domain_id;
    if (domain)
	*domain = rb_grn_object->domain;
    if (range_id)
	*range_id = rb_grn_object->range_id;
    if (range)
	*range = rb_grn_object->range;
}

/*
 * Document-method: close
 *
 * call-seq:
 *   object.close
 *
 * _object_が使用しているリソースを開放する。これ以降_object_を
 * 使うことはできない。
 */
VALUE
rb_grn_object_close (VALUE self)
{
    rb_grn_object_unbind(SELF(self));
    return Qnil;
}

/*
 * Document-method: closed?
 *
 * call-seq:
 *   object.closed? -> true/false
 *
 * _object_が開放済みの場合は+true+を返し、そうでない場合は
 * +false+を返す。
 */
static VALUE
rb_grn_object_closed_p (VALUE self)
{
    RbGrnObject *rb_grn_object;

    rb_grn_object = SELF(self);
    if (rb_grn_object->context && rb_grn_object->object)
        return Qfalse;
    else
        return Qtrue;
}

VALUE
rb_grn_object_inspect_object (VALUE inspected, grn_ctx *context, grn_obj *object)
{
    VALUE rb_object;

    rb_object = GRNOBJECT2RVAL(Qnil, context, object, RB_GRN_FALSE);
    rb_str_concat(inspected, rb_inspect(rb_object));

    return inspected;
}

VALUE
rb_grn_object_inspect_header (VALUE self, VALUE inspected)
{
    rb_str_cat2(inspected, "#<");
    rb_str_concat(inspected, rb_inspect(rb_obj_class(self)));
    rb_str_cat2(inspected, " ");

    return inspected;
}

static VALUE
rb_grn_object_inspect_content_id (VALUE inspected,
				  grn_ctx *context, grn_obj *object)
{
    grn_id id;

    rb_str_cat2(inspected, "id: <");
    id = grn_obj_id(context, object);
    if (id == GRN_ID_NIL)
	rb_str_cat2(inspected, "nil");
    else
	rb_str_concat(inspected, rb_obj_as_string(UINT2NUM(id)));
    rb_str_cat2(inspected, ">");

    return inspected;
}

static VALUE
rb_grn_object_inspect_content_name (VALUE inspected,
				    grn_ctx *context, grn_obj *object)
{
    int name_size;

    rb_str_cat2(inspected, "name: ");
    name_size = grn_obj_name(context, object, NULL, 0);
    if (name_size == 0) {
	rb_str_cat2(inspected, "(anonymous)");
    } else {
	grn_obj name;

	GRN_OBJ_INIT(&name, GRN_BULK, 0, GRN_ID_NIL);
	grn_bulk_space(context, &name, name_size);
	grn_obj_name(context, object, GRN_BULK_HEAD(&name), name_size);
	GRN_TEXT_PUTC(context, &name, '\0');
	rb_str_cat2(inspected, "<");
	rb_str_cat2(inspected, GRN_BULK_HEAD(&name));
	rb_str_cat2(inspected, ">");
	grn_obj_close(context, &name);
    }

    return inspected;
}

static VALUE
rb_grn_object_inspect_content_path (VALUE inspected,
				    grn_ctx *context, grn_obj *object)
{
    const char *path;

    rb_str_cat2(inspected, "path: ");
    path = grn_obj_path(context, object);
    if (path) {
	rb_str_cat2(inspected, "<");
	rb_str_cat2(inspected, path);
	rb_str_cat2(inspected, ">");
    } else {
	rb_str_cat2(inspected, "(temporary)");
    }

    return inspected;
}

static VALUE
rb_grn_object_inspect_content_domain (VALUE inspected,
				      grn_ctx *context, grn_obj *object)
{
    grn_id domain;

    rb_str_cat2(inspected, "domain: ");
    domain = object->header.domain;
    rb_str_cat2(inspected, "<");
    if (domain == GRN_ID_NIL) {
	rb_str_cat2(inspected, "nil");
    } else {
	grn_obj *domain_object;

	domain_object = grn_ctx_at(context, domain);
	if (domain_object) {
	    rb_grn_object_inspect_object(inspected, context, domain_object);
	} else {
	    rb_str_concat(inspected, rb_obj_as_string(UINT2NUM(domain)));
	}
    }
    rb_str_cat2(inspected, ">");

    return inspected;
}

static VALUE
rb_grn_object_inspect_content_range (VALUE inspected,
				     grn_ctx *context, grn_obj *object)
{
    grn_id range;

    rb_str_cat2(inspected, "range: ");

    range = grn_obj_get_range(context, object);
    rb_str_cat2(inspected, "<");
    switch (object->header.type) {
      case GRN_TYPE:
	rb_str_concat(inspected, rb_inspect(UINT2NUM(range)));
	break;
      default:
	if (range == GRN_ID_NIL) {
	    rb_str_cat2(inspected, "nil");
	} else {
	    grn_obj *range_object;

	    range_object = grn_ctx_at(context, range);
	    if (range_object) {
		rb_grn_object_inspect_object(inspected, context, range_object);
	    } else {
		rb_str_concat(inspected, rb_obj_as_string(UINT2NUM(range)));
	    }
	}
    }
    rb_str_cat2(inspected, ">");

    return inspected;
}

VALUE
rb_grn_object_inspect_object_content (VALUE inspected,
				      grn_ctx *context, grn_obj *object)
{
    rb_grn_object_inspect_content_id(inspected, context, object);
    rb_str_cat2(inspected, ", ");
    rb_grn_object_inspect_content_name(inspected, context, object);
    rb_str_cat2(inspected, ", ");
    rb_grn_object_inspect_content_path(inspected, context, object);
    rb_str_cat2(inspected, ", ");
    rb_grn_object_inspect_content_domain(inspected, context, object);
    rb_str_cat2(inspected, ", ");
    rb_grn_object_inspect_content_range(inspected, context, object);

    return inspected;
}

VALUE
rb_grn_object_inspect_content (VALUE self, VALUE inspected)
{
    RbGrnObject *rb_grn_object;
    grn_ctx *context;
    grn_obj *object;

    rb_grn_object = SELF(self);
    if (!rb_grn_object)
	return inspected;

    context = rb_grn_object->context;
    object = rb_grn_object->object;

    if (!object)
	return inspected;

    rb_grn_object_inspect_object_content(inspected, context, object);

    return inspected;
}

VALUE
rb_grn_object_inspect_footer (VALUE self, VALUE inspected)
{
    rb_str_cat2(inspected, ">");

    return inspected;
}

/*
 * Document-method: inspect
 *
 * call-seq:
 *   object.inspect -> 詳細情報
 *
 * _object_の詳細を示した文字列を返す。デバッグ用。
 */
static VALUE
rb_grn_object_inspect (VALUE self)
{
    VALUE inspected;

    inspected = rb_str_new2("");
    rb_grn_object_inspect_header(self, inspected);
    rb_grn_object_inspect_content(self, inspected);
    rb_grn_object_inspect_footer(self, inspected);

    return inspected;
}

/*
 * Document-method: id
 *
 * call-seq:
 *   object.id -> ID/nil
 *
 * _object_のIDを返す。_object_が#closed?なときやIDがない場合
 * は+nil+を返す。
 */
static VALUE
rb_grn_object_get_id (VALUE self)
{
    RbGrnObject *rb_grn_object;
    grn_id id;

    rb_grn_object = SELF(self);
    if (!rb_grn_object->object)
	return Qnil;

    id = grn_obj_id(rb_grn_object->context, rb_grn_object->object);
    if (id == GRN_ID_NIL)
	return Qnil;
    else
        return UINT2NUM(id);
}

/*
 * Document-method: domain
 *
 * call-seq:
 *   object.domain -> Groonga::Object/nil
 *
 * _object_の属しているGroonga::Objectを返す。例えば、
 * Groonga::ColumnはGroonga::Tableを返す。属している
 * Groonga::Objectがない場合は+nil+を返す。
 */
static VALUE
rb_grn_object_get_domain (VALUE self)
{
    RbGrnObject *rb_grn_object;
    grn_ctx *context;
    grn_obj *object;
    grn_id domain;

    rb_grn_object = SELF(self);
    object = rb_grn_object->object;
    if (!object)
	return Qnil;

    context = rb_grn_object->context;
    domain = object->header.domain;
    if (domain == GRN_ID_NIL) {
	return Qnil;
    } else {
	grn_obj *domain_object;

	domain_object = grn_ctx_at(context, domain);
	if (domain_object)
	    return GRNOBJECT2RVAL(Qnil, context, domain_object, RB_GRN_FALSE);
	else
	    return UINT2NUM(domain);
    }
}

/*
 * Document-method: name
 *
 * call-seq:
 *   object.name -> 名前/nil
 *
 * _object_の名前を返す。無名オブジェクトの場合は+nil+を返す。
 */
static VALUE
rb_grn_object_get_name (VALUE self)
{
    RbGrnObject *rb_grn_object;
    VALUE rb_name;
    int name_size;

    rb_grn_object = SELF(self);
    if (!rb_grn_object->object)
	return Qnil;

    name_size = grn_obj_name(rb_grn_object->context, rb_grn_object->object,
			     NULL, 0);
    if (name_size == 0)
	return Qnil;

    rb_name = rb_str_buf_new(name_size);
    rb_str_set_len(rb_name, name_size);
    grn_obj_name(rb_grn_object->context, rb_grn_object->object,
		 RSTRING_PTR(rb_name), name_size);
    return rb_name;
}

/*
 * Document-method: range
 *
 * call-seq:
 *   object.range -> Groonga::Object/nil
 *
 * _object_の値がとりうる範囲を示したGroonga::Objectを返す。
 * 例えば、Groonga::Columnの場合は
 * Groonga::Table#define_columnで指定されたGroonga::Typeや
 * Groonga::Tableを返す。
 * 範囲が指定されていないオブジェクトの場合は+nil+を返す。
 */
static VALUE
rb_grn_object_get_range (VALUE self)
{
    RbGrnObject *rb_grn_object;
    grn_ctx *context;
    grn_obj *object;
    grn_id range;

    rb_grn_object = SELF(self);
    object = rb_grn_object->object;
    if (!object)
	return Qnil;

    context = rb_grn_object->context;
    range = grn_obj_get_range(context, object);
    if (range == GRN_ID_NIL) {
	return Qnil;
    } else {
	grn_obj *range_object;

	range_object = grn_ctx_at(context, range);
	if (range_object)
	    return GRNOBJECT2RVAL(Qnil, context, range_object, RB_GRN_FALSE);
	else
	    return UINT2NUM(range);
    }
}

/*
 * Document-method: ==
 *
 * call-seq:
 *   object == other -> true/false
 *
 * _object_と_other_が同じgroongaのオブジェクトなら+true+を返
 * し、そうでなければ+false+を返す。
 */
static VALUE
rb_grn_object_equal (VALUE self, VALUE other)
{
    RbGrnObject *self_rb_grn_object, *other_rb_grn_object;

    if (self == other)
        return Qtrue;

    if (!RVAL2CBOOL(rb_funcall(rb_obj_class(self), rb_intern("=="), 1,
                               rb_obj_class(other))))
        return Qfalse;

    self_rb_grn_object = SELF(self);
    other_rb_grn_object = SELF(other);

    return self_rb_grn_object->object == other_rb_grn_object->object;
}

/*
 * Document-method: []
 *
 * call-seq:
 *   object[id] -> 値
 *
 * _object_の_id_に対応する値を返す。
 */
VALUE
rb_grn_object_array_reference (VALUE self, VALUE rb_id)
{
    VALUE exception;
    RbGrnObject *rb_grn_object;
    grn_id id, range_id;
    grn_ctx *context;
    grn_obj *object;
    grn_obj *range;
    unsigned char range_type;
    grn_obj value;
    VALUE rb_value = Qnil;

    rb_grn_object = SELF(self);
    context = rb_grn_object->context;
    object = rb_grn_object->object;
    if (!object)
	return Qnil;

    id = NUM2UINT(rb_id);
    range_id = grn_obj_get_range(context, object);
    range = grn_ctx_at(context, range_id);
    range_type = range ? range->header.type : GRN_VOID;
    switch (object->header.type) {
      case GRN_TABLE_HASH_KEY:
      case GRN_TABLE_PAT_KEY:
      case GRN_TABLE_NO_KEY:
	GRN_OBJ_INIT(&value, GRN_BULK, 0, GRN_ID_NIL);
	break;
      case GRN_TYPE:
      case GRN_ACCESSOR: /* FIXME */
	GRN_OBJ_INIT(&value, GRN_BULK, 0, range_id);
	break;
      case GRN_COLUMN_VAR_SIZE:
      case GRN_COLUMN_FIX_SIZE:
	switch (object->header.flags & GRN_OBJ_COLUMN_TYPE_MASK) {
	  case GRN_OBJ_COLUMN_VECTOR:
	    GRN_OBJ_INIT(&value, GRN_VECTOR, 0, range_id);
	    break;
	  case GRN_OBJ_COLUMN_INDEX:
	  case GRN_OBJ_COLUMN_SCALAR:
	    GRN_OBJ_INIT(&value, GRN_BULK, 0, range_id);
	    break;
	  default:
	    rb_raise(rb_eGrnError, "unsupported column type: %u: %s",
		     range_type, rb_grn_inspect(self));
	    break;
	}
	break;
      default:
	rb_raise(rb_eGrnError,
		 "unsupported type: %s", rb_grn_inspect(self));
	break;
    }

    grn_obj_get_value(context, object, id, &value);
    exception = rb_grn_context_to_exception(context, self);
    if (NIL_P(exception))
	rb_value = GRNVALUE2RVAL(context, &value, range, self);
    grn_obj_close(context, &value);
    if (!NIL_P(exception))
	rb_exc_raise(exception);

    return rb_value;
}

static VALUE
rb_grn_object_set (VALUE self, VALUE rb_id, VALUE rb_value, int flags)
{
    RbGrnObject *rb_grn_object;
    grn_ctx *context;
    grn_id id;
    grn_obj value;
    grn_rc rc;
    VALUE exception;

    rb_grn_object = SELF(self);
    if (!rb_grn_object->object)
	return Qnil;

    context = rb_grn_object->context;
    id = NUM2UINT(rb_id);
    if (RVAL2CBOOL(rb_obj_is_kind_of(rb_value, rb_cArray))) {
	GRN_OBJ_INIT(&value, GRN_VECTOR, 0, GRN_ID_NIL);
	RVAL2GRNVECTOR(rb_value, context, &value);
    } else {
	GRN_OBJ_INIT(&value, GRN_BULK, 0, GRN_ID_NIL);
	RVAL2GRNBULK(rb_value, context, &value);
    }
    rc = grn_obj_set_value(context, rb_grn_object->object, id,
			   &value, flags);
    exception = rb_grn_context_to_exception(context, self);
    grn_obj_close(context, &value);
    if (!NIL_P(exception))
	rb_exc_raise(exception);
    rb_grn_rc_check(rc, self);

    return Qnil;
}

/*
 * Document-method: []=
 *
 * call-seq:
 *   object[id] = value
 *
 * _object_の_id_に対応する値を設定する。既存の値は上書きさ
 * れる。
 */
static VALUE
rb_grn_object_array_set (VALUE self, VALUE rb_id, VALUE rb_value)
{
    return rb_grn_object_set(self, rb_id, rb_value, GRN_OBJ_SET);
}

/*
 * Document-method: append
 *
 * call-seq:
 *   object.append(id, value)
 *
 * _object_の_id_に対応する値に_value_を追加する。
 */
static VALUE
rb_grn_object_append_value (VALUE self, VALUE rb_id, VALUE rb_value)
{
    return rb_grn_object_set(self, rb_id, rb_value, GRN_OBJ_APPEND);
}

static VALUE
rb_grn_object_remove (VALUE self)
{
    RbGrnObject *rb_grn_object;
    grn_ctx *context;
    grn_rc rc;

    rb_grn_object = SELF(self);
    if (!rb_grn_object->object)
	return Qnil;

    context = rb_grn_object->context;
    rc = grn_obj_remove(context, rb_grn_object->object);
    rb_grn_rc_check(rc, self);

    rb_grn_object->unbind(rb_grn_object);
    rb_iv_set(self, "context", Qnil);

    return Qnil;
}

void
rb_grn_init_object (VALUE mGrn)
{
    rb_cGrnObject = rb_define_class_under(mGrn, "Object", rb_cObject);
    rb_define_alloc_func(rb_cGrnObject, rb_grn_object_alloc);

    rb_define_method(rb_cGrnObject, "inspect", rb_grn_object_inspect, 0);

    rb_define_method(rb_cGrnObject, "id", rb_grn_object_get_id, 0);
    rb_define_method(rb_cGrnObject, "domain", rb_grn_object_get_domain, 0);
    rb_define_method(rb_cGrnObject, "name", rb_grn_object_get_name, 0);
    rb_define_method(rb_cGrnObject, "range", rb_grn_object_get_range, 0);

    rb_define_method(rb_cGrnObject, "==", rb_grn_object_equal, 1);

    rb_define_method(rb_cGrnObject, "close", rb_grn_object_close, 0);
    rb_define_method(rb_cGrnObject, "closed?", rb_grn_object_closed_p, 0);

    rb_define_method(rb_cGrnObject, "[]", rb_grn_object_array_reference, 1);
    rb_define_method(rb_cGrnObject, "[]=", rb_grn_object_array_set, 2);
    rb_define_method(rb_cGrnObject, "append", rb_grn_object_append_value, 2);

    rb_define_method(rb_cGrnObject, "remove", rb_grn_object_remove, 0);
}