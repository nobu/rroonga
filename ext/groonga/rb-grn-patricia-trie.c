/* -*- c-file-style: "ruby" -*- */
/* vim: set sts=4 sw=4 ts=8 noet: */
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

#define SELF(object) ((RbGrnTableKeySupport *)DATA_PTR(object))

VALUE rb_cGrnPatriciaTrie;

/*
 * Document-class: Groonga::PatriciaTrie < Groonga::Table
 *
 * 各レコードをパトリシアトライで管理するテーブル。ハッシュ
 * テーブルに比べて完全一致検索の速度がやや遅いが、前方一致
 * 検索・共通接頭辞探索などの検索ができる。またカーソルを用
 * いてキーの昇降順にレコードを取り出すことができる。
 */

/*
 * call-seq:
 *   Groonga::PatriciaTrie.create(options={}) -> Groonga::PatriciaTrie
 *   Groonga::PatriciaTrie.create(options={}) {|table| ... }
 *
 * 各レコードをパトリシアトライで管理するテーブルを生成する。
 * ブロックを指定すると、そのブロックに生成したテーブルが渡さ
 * れ、ブロックを抜けると自動的にテーブルが破棄される。
 *
 * _options_に指定可能な値は以下の通り。
 *
 * [+:context+]
 *   テーブルが利用するGroonga::Context。省略すると
 *   Groonga::Context.defaultを用いる。
 *
 * [+:name+]
 *   テーブルの名前。名前をつけると、Groonga::Context#[]に名
 *   前を指定してテーブルを取得することができる。省略すると
 *   無名テーブルになり、テーブルIDでのみ取得できる。
 *
 * [+:path+]
 *   テーブルを保存するパス。パスを指定すると永続テーブルとな
 *   り、プロセス終了後もレコードは保持される。次回起動時に
 *   Groonga::Context#[]で保存されたレコードを利用する
 *   ことができる。省略すると一時テーブルになり、プロセスが終
 *   了するとレコードは破棄される。
 *
 * [+:persistent+]
 *   +true+を指定すると永続テーブルとなる。+path+を省略した
 *   場合は自動的にパスが付加される。+:context+で指定した
 *   Groonga::Contextに結びついているデータベースが一時デー
 *   タベースの場合は例外が発生する。
 *
 * [+:key_normalize+]
 *   +true+を指定するとキーを正規化する。
 *
 * [+:key_with_sis+]
 *   +true+を指定するとキーの文字列の全suffixが自動的に登
 *   録される。
 *
 * [+:key_type+]
 *   キーの種類を示すオブジェクトを指定する。キーの種類には型
 *   名（"Int32"や"ShortText"など）またはGroonga::Typeまたは
 *   テーブル（Groonga::Array、Groonga::Hash、
 *   Groonga::PatriciaTrieのどれか）を指定する。
 *
 *   Groonga::Typeを指定した場合は、その型が示す範囲の値をキー
 *   として使用する。ただし、キーの最大サイズは4096バイトで
 *   あるため、Groonga::Type::TEXTやGroonga::Type::LONG_TEXT
 *   は使用できない。
 *
 *   テーブルを指定した場合はレコードIDをキーとして使用する。
 *   指定したテーブルのGroonga::Recordをキーとして使用するこ
 *   ともでき、その場合は自動的にGroonga::Recordからレコード
 *   IDを取得する。
 *
 *   省略した場合は文字列をキーとして使用する。この場合、
 *   4096バイトまで使用可能である。
 *
 * [+:value_type+]
 *   値の型を指定する。省略すると値のための領域を確保しない。
 *   値を保存したい場合は必ず指定すること。
 *
 *   参考: Groonga::Type.new
 *
 * [+:default_tokenizer+]
 *   Groonga::IndexColumnで使用するトークナイザを指定する。
 *   デフォルトでは何も設定されていないので、テーブルに
 *   Groonga::IndexColumnを定義する場合は
 *   <tt>"TokenBigram"</tt>などを指定する必要がある。
 *
 * [+:sub_records+]
 *   +true+を指定すると#groupでグループ化したときに、
 *   Groonga::Record#n_sub_recordsでグループに含まれるレコー
 *   ドの件数を取得できる。
 *
 * 使用例:
 *
 * 無名一時テーブルを生成する。
 *   Groonga::PatriciaTrie.create
 *
 * 無名永続テーブルを生成する。
 *   Groonga::PatriciaTrie.create(:path => "/tmp/hash.grn")
 *
 * 名前付き永続テーブルを生成する。ただし、ファイル名は気に
 * しない。
 *   Groonga::PatriciaTrie.create(:name => "Bookmarks",
 *                                :persistent => true)
 *
 * それぞれのレコードに512バイトの値を格納できる無名一時テー
 * ブルを生成する。
 *   Groonga::PatriciaTrie.create(:value => 512)
 *
 * キーとして文字列を使用する無名一時テーブルを生成する。
 *   Groonga::PatriciaTrie.create(:key_type => Groonga::Type::SHORT_TEXT)
 *
 * キーとして文字列を使用する無名一時テーブルを生成する。
 * （キーの種類を表すオブジェクトは文字列で指定。）
 *   Groonga::PatriciaTrie.create(:key_type => "ShortText")
 *
 * キーとして<tt>Bookmarks</tt>テーブルのレコードを使用す
 * る無名一時テーブルを生成する。
 *   bookmarks = Groonga::PatriciaTrie.create(:name => "Bookmarks")
 *   Groonga::PatriciaTrie.create(:key_type => bookmarks)
 *
 * キーとして<tt>Bookmarks</tt>テーブルのレコードを使用す
 * る無名一時テーブルを生成する。
 * （テーブルは文字列で指定。）
 *   Groonga::PatriciaTrie.create(:name => "Bookmarks")
 *   Groonga::PatriciaTrie.create(:key_type => "Bookmarks")
 *
 * 全文検索用のトークンをバイグラムで切り出す無名一時テーブ
 * ルを生成する。
 *   bookmarks = Groonga::PatriciaTrie.create(:name => "Bookmarks")
 *   bookmarks.define_column("comment", "Text")
 *   terms = Groonga::PatriciaTrie.create(:name => "Terms",
 *                                        :default_tokenizer => "TokenBigram")
 *   terms.define_index_column("content", bookmarks,
 *                             :source => "Bookmarks.comment")
 */
static VALUE
rb_grn_patricia_trie_s_create (int argc, VALUE *argv, VALUE klass)
{
    grn_ctx *context;
    grn_obj *key_type = NULL, *value_type = NULL, *table;
    const char *name = NULL, *path = NULL;
    unsigned name_size = 0;
    grn_obj_flags flags = GRN_TABLE_PAT_KEY;
    VALUE rb_table;
    VALUE options, rb_context, rb_name, rb_path, rb_persistent;
    VALUE rb_key_normalize, rb_key_with_sis, rb_key_type;
    VALUE rb_value_type;
    VALUE rb_default_tokenizer, rb_sub_records;

    rb_scan_args(argc, argv, "01", &options);

    rb_grn_scan_options(options,
			"context", &rb_context,
			"name", &rb_name,
                        "path", &rb_path,
			"persistent", &rb_persistent,
			"key_normalize", &rb_key_normalize,
			"key_with_sis", &rb_key_with_sis,
			"key_type", &rb_key_type,
			"value_type", &rb_value_type,
			"default_tokenizer", &rb_default_tokenizer,
			"sub_records", &rb_sub_records,
			NULL);

    context = rb_grn_context_ensure(&rb_context);

    if (!NIL_P(rb_name)) {
        name = StringValuePtr(rb_name);
	name_size = RSTRING_LEN(rb_name);
	flags |= GRN_OBJ_PERSISTENT;
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

    if (!NIL_P(rb_value_type))
	value_type = RVAL2GRNOBJECT(rb_value_type, &context);

    if (RVAL2CBOOL(rb_sub_records))
	flags |= GRN_OBJ_WITH_SUBREC;

    table = grn_table_create(context, name, name_size, path,
			     flags, key_type, value_type);
    if (!table)
	rb_grn_context_check(context, rb_ary_new4(argc, argv));
    rb_table = GRNOBJECT2RVAL(klass, context, table, RB_GRN_TRUE);

    if (!NIL_P(rb_default_tokenizer))
	rb_funcall(rb_table, rb_intern("default_tokenizer="), 1,
		   rb_default_tokenizer);

    if (rb_block_given_p())
        return rb_ensure(rb_yield, rb_table, rb_grn_object_close, rb_table);
    else
        return rb_table;
}

/*
 * call-seq:
 *   patricia_trie.search(key, options=nil) -> Groonga::Hash
 *
 * _key_にマッチするレコードのIDがキーに入っている
 * Groonga::Hashを返す。マッチするレコードがない場合は空の
 * Groonga::Hashが返る。
 *
 * _options_で+:result+を指定することにより、そのテーブルにマッ
 * チしたレコードIDがキーのレコードを追加することができる。
 * +:result+にテーブルを指定した場合は、そのテーブルが返る。
 *
 * _options_に指定可能な値は以下の通り。
 *
 * [+:result+]
 *   結果を格納するテーブル。
 *
 * [+:operator+]
 *   マッチしたレコードをどのように扱うか。指定可能な値は以
 *   下の通り。省略した場合はGroonga::Operation::OR。
 *
 *   [Groonga::Operation::OR]
 *     マッチしたレコードを追加。すでにレコードが追加され
 *     ている場合は何もしない。
 *   [Groonga::Operation::AND]
 *     マッチしたレコードのスコアを増加。マッチしなかった
 *     レコードを削除。
 *   [Groonga::Operation::BUT]
 *     マッチしたレコードを削除。
 *   [Groonga::Operation::ADJUST]
 *     マッチしたレコードのスコアを増加。
 *
 * [+:type+]
 *   ?????
 *
 * 複数のキーで検索し、結果を1つのテーブルに集める。
 *   result = nil
 *   keys = ["morita", "gunyara-kun", "yu"]
 *   keys.each do |key|
 *     result = users.search(key, :result => result)
 *   end
 *   result.each do |record|
 *     user = record.key
 *     p user.key # -> "morita"または"gunyara-kun"または"yu"
 *   end
 */
static VALUE
rb_grn_patricia_trie_search (int argc, VALUE *argv, VALUE self)
{
    grn_rc rc;
    grn_ctx *context;
    grn_obj *table;
    grn_id domain_id;
    grn_obj *key, *domain, *result;
    grn_operator operator;
    grn_search_optarg search_options;
    rb_grn_boolean search_options_is_set = RB_GRN_FALSE;
    VALUE rb_key, options, rb_result, rb_operator, rb_type;

    rb_grn_table_key_support_deconstruct(SELF(self), &table, &context,
					 &key, &domain_id, &domain,
					 NULL, NULL, NULL,
					 NULL);

    rb_scan_args(argc, argv, "11", &rb_key, &options);

    RVAL2GRNKEY(rb_key, context, key, domain_id, domain, self);

    rb_grn_scan_options(options,
			"result", &rb_result,
			"operator", &rb_operator,
			"type", &rb_type,
			NULL);

    if (NIL_P(rb_result)) {
	result = grn_table_create(context, NULL, 0, NULL,
				  GRN_OBJ_TABLE_HASH_KEY | GRN_OBJ_WITH_SUBREC,
				  table, 0);
	rb_grn_context_check(context, self);
	rb_result = GRNOBJECT2RVAL(Qnil, context, result, RB_GRN_TRUE);
    } else {
	result = RVAL2GRNOBJECT(rb_result, &context);
    }

    operator = RVAL2GRNOPERATOR(rb_operator);

    rc = grn_obj_search(context, table, key,
			result, operator,
			search_options_is_set ? &search_options : NULL);
    rb_grn_rc_check(rc, self);

    return rb_result;
}

/*
 * call-seq:
 *   patricia_trie.scan(string) -> Array
 *   patricia_trie.scan(string) {|record, word, start, length| ... }
 *
 * _string_を走査し、_patricia_trie_内に格納されているキーに
 * マッチした部分文字列の情報をブロックに渡す。複数のキーが
 * マッチする場合は最長一致するキーを優先する。
 *
 * [_record_]
 *   マッチしたキーのGroonga::Record。
 *
 * [_word_]
 *   マッチした部分文字列。
 *
 * [_start_]
 *   _string_内での_word_の出現位置。（バイト単位）
 *
 * [_length_]
 *   _word_の長さ。（バイト探知）
 *
 * ブロックを指定しない場合は、マッチした部分文字列の情報を
 * まとめて配列として返す。
 *
 *   words = Groonga::PatriciaTrie.create(:key_type => "ShortText",
 *                                        :key_normalize => true)
 *   words.add("リンク")
 *   adventure_of_link = words.add('リンクの冒険')
 *   words.add('冒険')
 *   gaxtu = words.add('ｶﾞｯ')
 *   muteki = words.add('ＭＵＴＥＫＩ')
 *
 *   text = 'muTEki リンクの冒険 ミリバール ガッ'
 *   words.scan(text).each do |record, word, start, length|
 *     p [record.key, word, start, length]
 *       # -> ["ＭＵＴＥＫＩ", "muTEki", 0, 6]
 *       # -> ["リンクの冒険", "リンクの冒険", 7, 18]
 *       # -> ["ｶﾞｯ", "ガッ", 42, 6]
 *   end
 *
 *   words.scan(text)
 *     # -> [[muteki, "muTEki", 0, 6],
 *     #     [adventure_of_link, "リンクの冒険", 7, 18],
 *     #     [gaxtu, "ガッ", 42, 6]]
 */
static VALUE
rb_grn_patricia_trie_scan (VALUE self, VALUE rb_string)
{
    grn_ctx *context;
    grn_obj *table;
    VALUE rb_result = Qnil;
    grn_pat_scan_hit hits[1024];
    const char *string;
    long string_length;
    rb_grn_boolean block_given;

    string = StringValuePtr(rb_string);
    string_length = RSTRING_LEN(rb_string);

    rb_grn_table_key_support_deconstruct(SELF(self), &table, &context,
					 NULL, NULL, NULL,
					 NULL, NULL, NULL,
					 NULL);

    block_given = rb_block_given_p();
    if (!block_given)
	rb_result = rb_ary_new();

    while (string_length > 0) {
	const char *rest;
	int i, n_hits;
	unsigned int previous_offset = 0;

	n_hits = grn_pat_scan(context, (grn_pat *)table,
			      string, string_length,
			      hits, sizeof(hits) / sizeof(*hits),
			      &rest);
	for (i = 0; i < n_hits; i++) {
	    VALUE record, term, matched_info;

	    if (hits[i].offset < previous_offset)
		continue;

	    record = rb_grn_record_new(self, hits[i].id, Qnil);
	    term = rb_grn_context_rb_string_new(context,
						string + hits[i].offset,
						hits[i].length);
	    matched_info = rb_ary_new3(4,
				       record,
				       term,
				       UINT2NUM(hits[i].offset),
				       UINT2NUM(hits[i].length));
	    if (block_given) {
		rb_yield(matched_info);
	    } else {
		rb_ary_push(rb_result, matched_info);
	    }
	    previous_offset = hits[i].offset;
	}
	string_length -= rest - string;
	string = rest;
    }

    return rb_result;
}

/*
 * call-seq:
 *   patricia_trie.prefix_search(prefix) -> Groonga::Hash
 *
 * キーが_prefix_に前方一致するレコードのIDがキーに入っている
 * Groonga::Hashを返す。マッチするレコードがない場合は空の
 * Groonga::Hashが返る。
 *
 */
static VALUE
rb_grn_patricia_trie_prefix_search (VALUE self, VALUE rb_prefix)
{
    grn_ctx *context;
    grn_obj *table, *key, *domain, *result;
    grn_id domain_id;
    VALUE rb_result;

    rb_grn_table_key_support_deconstruct(SELF(self), &table, &context,
					 &key, &domain_id, &domain,
					 NULL, NULL, NULL,
					 NULL);

    result = grn_table_create(context, NULL, 0, NULL,
			      GRN_OBJ_TABLE_HASH_KEY,
			      table, 0);
    rb_grn_context_check(context, self);
    rb_result = GRNOBJECT2RVAL(Qnil, context, result, RB_GRN_TRUE);

    GRN_BULK_REWIND(key);
    RVAL2GRNKEY(rb_prefix, context, key, domain_id, domain, self);
    grn_pat_prefix_search(context, (grn_pat *)table,
			  GRN_BULK_HEAD(key), GRN_BULK_VSIZE(key),
			  (grn_hash *)result);
    rb_grn_context_check(context, self);

    return rb_result;
}

void
rb_grn_init_patricia_trie (VALUE mGrn)
{
    rb_cGrnPatriciaTrie =
	rb_define_class_under(mGrn, "PatriciaTrie", rb_cGrnTable);

    rb_include_module(rb_cGrnPatriciaTrie, rb_mGrnTableKeySupport);
    rb_define_singleton_method(rb_cGrnPatriciaTrie, "create",
			       rb_grn_patricia_trie_s_create, -1);

    rb_define_method(rb_cGrnPatriciaTrie, "search",
		     rb_grn_patricia_trie_search, -1);
    rb_define_method(rb_cGrnPatriciaTrie, "scan",
		     rb_grn_patricia_trie_scan, 1);
    rb_define_method(rb_cGrnPatriciaTrie, "prefix_search",
		     rb_grn_patricia_trie_prefix_search, 1);
}
