# Copyright (C) 2009-2010  Kouhei Sutou <kou@clear-code.com>
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License version 2.1 as published by the Free Software Foundation.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

class SchemaTest < Test::Unit::TestCase
  include GroongaTestUtils

  setup :setup_database

  def test_create_table
    assert_nil(context["Posts"])
    Groonga::Schema.create_table("Posts") do |table|
    end
    assert_not_nil(context["Posts"])
  end

  def test_create_table_force
    Groonga::Schema.create_table("Posts") do |table|
      table.string("name")
    end
    assert_not_nil(context["Posts.name"])

    Groonga::Schema.create_table("Posts") do |table|
    end
    assert_not_nil(context["Posts.name"])

    Groonga::Schema.create_table("Posts", :force => true) do |table|
    end
    assert_nil(context["Posts.name"])
  end

  def test_remove_table
    Groonga::Array.create(:name => "Posts")
    assert_not_nil(context["Posts"])
    Groonga::Schema.remove_table("Posts")
    assert_nil(context["Posts"])
  end

  def test_define_hash
    Groonga::Schema.create_table("Posts", :type => :hash) do |table|
    end
    assert_kind_of(Groonga::Hash, context["Posts"])
  end

  def test_define_hash_with_full_option
    path = @tmp_dir + "hash.groonga"
    tokenizer = context["TokenTrigram"]
    Groonga::Schema.create_table("Posts",
                                 :type => :hash,
                                 :key_type => "integer",
                                 :path => path.to_s,
                                 :value_type => "UInt32",
                                 :default_tokenizer => tokenizer) do |table|
    end
    table = context["Posts"]
    assert_equal("#<Groonga::Hash " +
                 "id: <#{table.id}>, " +
                 "name: <Posts>, " +
                 "path: <#{path}>, " +
                 "domain: <Int32>, " +
                 "range: <UInt32>, " +
                 "flags: <>, " +
                 "encoding: <#{Groonga::Encoding.default.inspect}>, " +
                 "size: <0>>",
                 table.inspect)
    assert_equal(tokenizer, table.default_tokenizer)
  end

  def test_define_patricia_trie
    Groonga::Schema.create_table("Posts", :type => :patricia_trie) do |table|
    end
    assert_kind_of(Groonga::PatriciaTrie, context["Posts"])
  end

  def test_define_patricia_trie_with_full_option
    path = @tmp_dir + "patricia-trie.groonga"
    Groonga::Schema.create_table("Posts",
                                 :type => :patricia_trie,
                                 :key_type => "integer",
                                 :path => path.to_s,
                                 :value_type => "Float",
                                 :default_tokenizer => "TokenBigram",
                                 :key_normalize => true,
                                 :key_with_sis => true) do |table|
    end
    table = context["Posts"]
    assert_equal("#<Groonga::PatriciaTrie " +
                 "id: <#{table.id}>, " +
                 "name: <Posts>, " +
                 "path: <#{path}>, " +
                 "domain: <Int32>, " +
                 "range: <Float>, " +
                 "flags: <KEY_WITH_SIS|KEY_NORMALIZE|WITH_SECTION>, " +
                 "encoding: <#{Groonga::Encoding.default.inspect}>, " +
                 "size: <0>>",
                 table.inspect)
    assert_equal(context["TokenBigram"], table.default_tokenizer)
  end

  def test_define_array
    Groonga::Schema.create_table("Posts", :type => :array) do |table|
    end
    assert_kind_of(Groonga::Array, context["Posts"])
  end

  def test_define_array_with_full_option
    path = @tmp_dir + "array.groonga"
    Groonga::Schema.create_table("Posts",
                                 :type => :array,
                                 :path => path.to_s,
                                 :value_type => "Int32") do |table|
    end
    table = context["Posts"]
    assert_equal("#<Groonga::Array " +
                 "id: <#{table.id}>, " +
                 "name: <Posts>, " +
                 "path: <#{path}>, " +
                 "domain: <Int32>, " +
                 "range: <Int32>, " +
                 "flags: <>, " +
                 "size: <0>>",
                 table.inspect)
  end

  def test_column_with_full_option
    path = @tmp_dir + "column.groonga"
    type = Groonga::Type.new("Niku", :size => 29)
    Groonga::Schema.create_table("Posts") do |table|
      table.column("rate",
                   type,
                   :path => path.to_s,
                   :persistent => true,
                   :type => :vector,
                   :compress => :lzo)
    end

    column_name = "Posts.rate"
    column = context[column_name]
    assert_equal("#<Groonga::VariableSizeColumn " +
                 "id: <#{column.id}>, " +
                 "name: <#{column_name}>, " +
                 "path: <#{path}>, " +
                 "domain: <Posts>, " +
                 "range: <Niku>, " +
                 "flags: <COMPRESS_LZO>>",
                 column.inspect)
  end

  def test_integer32_column
    assert_nil(context["Posts.rate"])
    Groonga::Schema.create_table("Posts") do |table|
      table.integer32 :rate
    end
    assert_equal(context["Int32"], context["Posts.rate"].range)
  end

  def test_integer64_column
    assert_nil(context["Posts.rate"])
    Groonga::Schema.create_table("Posts") do |table|
      table.integer64 :rate
    end
    assert_equal(context["Int64"], context["Posts.rate"].range)
  end

  def test_unsigned_integer32_column
    assert_nil(context["Posts.n_viewed"])
    Groonga::Schema.create_table("Posts") do |table|
      table.unsigned_integer32 :n_viewed
    end
    assert_equal(context["UInt32"], context["Posts.n_viewed"].range)
  end

  def test_unsigned_integer64_column
    assert_nil(context["Posts.n_viewed"])
    Groonga::Schema.create_table("Posts") do |table|
      table.unsigned_integer64 :n_viewed
    end
    assert_equal(context["UInt64"], context["Posts.n_viewed"].range)
  end

  def test_float_column
    assert_nil(context["Posts.rate"])
    Groonga::Schema.create_table("Posts") do |table|
      table.float :rate
    end
    assert_equal(context["Float"], context["Posts.rate"].range)
  end

  def test_time_column
    assert_nil(context["Posts.last_modified"])
    Groonga::Schema.create_table("Posts") do |table|
      table.time :last_modified
    end
    assert_equal(context["Time"], context["Posts.last_modified"].range)
  end

  def test_short_text_column
    assert_nil(context["Posts.title"])
    Groonga::Schema.create_table("Posts") do |table|
      table.short_text :title
    end
    assert_equal(context["ShortText"], context["Posts.title"].range)
  end

  def test_text_column
    assert_nil(context["Posts.comment"])
    Groonga::Schema.create_table("Posts") do |table|
      table.text :comment
    end
    assert_equal(context["Text"], context["Posts.comment"].range)
  end

  def test_long_text_column
    assert_nil(context["Posts.content"])
    Groonga::Schema.create_table("Posts") do |table|
      table.long_text :content
    end
    assert_equal(context["LongText"], context["Posts.content"].range)
  end

  def test_boolean_column
    assert_nil(context["Posts.public"])
    Groonga::Schema.create_table("Posts") do |table|
      table.boolean :public
    end
    assert_equal(context["Bool"], context["Posts.public"].range)
  end

  def test_remove_column
    Groonga::Schema.create_table("Posts") do |table|
      table.long_text :content
    end
    assert_not_nil(context["Posts.content"])

    Groonga::Schema.change_table("Posts") do |table|
      table.remove_column("content")
    end
    assert_nil(context["Posts.content"])
  end

  def test_column_again
    Groonga::Schema.create_table("Posts") do |table|
      table.text :content
    end

    assert_nothing_raised do
      Groonga::Schema.create_table("Posts") do |table|
        table.text :content
      end
    end
  end

  def test_column_again_with_difference_type
    Groonga::Schema.create_table("Posts") do |table|
      table.text :content
    end

    assert_raise(ArgumentError) do
      Groonga::Schema.create_table("Posts") do |table|
        table.integer :content
      end
    end
  end

  def test_index
    assert_nil(context["Terms.content"])
    Groonga::Schema.create_table("Posts") do |table|
      table.long_text :content
    end
    Groonga::Schema.create_table("Terms") do |table|
      table.index "Posts.content"
    end
    assert_equal([context["Posts.content"]],
                 context["Terms.Posts_content"].sources)
  end

  def test_index_with_full_option
    path = @tmp_dir + "index-column.groonga"
    assert_nil(context["Terms.content"])
    index_column_name = "Posts_index"

    Groonga::Schema.create_table("Posts") do |table|
      table.long_text :content
    end
    Groonga::Schema.create_table("Terms") do |table|
      table.index("Posts.content",
                  :name => index_column_name,
                  :path => path.to_s,
                  :persistent => true,
                  :with_section => true,
                  :with_weight => true,
                  :with_position => true)
    end

    posts = context["Posts"]
    terms = context["Terms"]
    full_index_column_name = "Terms.#{index_column_name}"
    index_column = context[full_index_column_name]
    assert_equal("#<Groonga::IndexColumn " +
                 "id: <#{index_column.id}>, " +
                 "name: <#{full_index_column_name}>, " +
                 "path: <#{path}>, " +
                 "domain: <Terms>, " +
                 "range: <Posts>, " +
                 "flags: <WITH_SECTION|WITH_WEIGHT|WITH_POSITION>>",
                 index_column.inspect)
  end

  def test_index_again
    Groonga::Schema.create_table("Posts") do |table|
      table.long_text :content
    end
    Groonga::Schema.create_table("Terms") do |table|
      table.index "Posts.content"
    end

    assert_nothing_raised do
      Groonga::Schema.create_table("Terms") do |table|
        table.index "Posts.content"
      end
    end
  end

  def test_index_again_with_difference_source
    Groonga::Schema.create_table("Posts") do |table|
      table.long_text :content
      table.short_text :name
    end
    Groonga::Schema.create_table("Terms") do |table|
      table.index "Posts.content"
    end

    assert_raise(ArgumentError) do
      Groonga::Schema.create_table("Terms") do |table|
        table.index "Posts.name", :name => "Posts_content"
      end
    end
  end

  def test_index_key
    Groonga::Schema.create_table("Posts",
                                 :type => :hash,
                                 :key_type => "ShortText") do |table|
    end
    Groonga::Schema.create_table("Terms") do |table|
      table.index "Posts._key", :with_position => true
    end

    full_index_column_name = "Terms.Posts__key"
    index_column = context[full_index_column_name]
    assert_equal("#<Groonga::IndexColumn " +
                 "id: <#{index_column.id}>, " +
                 "name: <#{full_index_column_name}>, " +
                 "path: <#{index_column.path}>, " +
                 "domain: <Terms>, " +
                 "range: <Posts>, " +
                 "flags: <WITH_POSITION>>",
                 index_column.inspect)
  end

  def test_index_key_again
    Groonga::Schema.create_table("Posts",
                                 :type => :hash,
                                 :key_type => "ShortText") do |table|
    end
    Groonga::Schema.create_table("Terms") do |table|
      table.index "Posts._key", :with_position => true
    end

    assert_nothing_raised do
      Groonga::Schema.create_table("Terms") do |table|
        table.index "Posts._key"
      end
    end
  end

  def test_dump
    Groonga::Schema.define do |schema|
      schema.create_table("Posts") do |table|
        table.short_text :title
      end
    end
    assert_equal(<<-EOS, Groonga::Schema.dump)
create_table("Posts") do |table|
  table.short_text("title")
end
EOS
  end

  def test_reference_dump
    Groonga::Schema.define do |schema|
      schema.create_table("Items") do |table|
        table.short_text("title")
      end

      schema.create_table("Users") do |table|
        table.short_text("name")
      end

      schema.create_table("Comments") do |table|
        table.reference("item", "Items")
        table.reference("author", "Users")
        table.text("content")
        table.time("issued")
      end
    end

    assert_equal(<<-EOS, Groonga::Schema.dump)
create_table("Comments") do |table|
  table.text("content")
  table.time("issued")
end

create_table("Items") do |table|
  table.short_text("title")
end

create_table("Users") do |table|
  table.short_text("name")
end

change_table("Comments") do |table|
  table.reference("author", "Users")
  table.reference("item", "Items")
end
EOS
  end

  def test_explicit_context_create_table
    context = Groonga::Context.default
    Groonga::Context.default = nil

    Groonga::Schema.define(:context => context) do |schema|
      schema.create_table('Items', :type => :hash) do |table|
        table.text("text")
      end
      schema.create_table("TermsText",
                          :type => :patricia_trie,
                          :key_normalize => true,
                          :default_tokenizer => "TokenBigram") do |table|
        table.index('Items.text')
      end
    end

    assert_not_nil(context["Items.text"])
    assert_not_nil(context["TermsText.Items_text"])
  end
end
