# -*- coding: utf-8 -*-
#
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

module Groonga
  class Record
    # レコードが所属するテーブル
    attr_reader :table
    # レコードのID
    attr_reader :id
    # _table_の_id_に対応するレコードを作成する。_values_には各
    # カラムに設定する値を以下のような形式で指定する。
    #
    #   [
    #    ["カラム名", 値],
    #    ["カラム名", 値],
    #    ...,
    #   ]
    def initialize(table, id, values=nil)
      @table = table
      @id = id
      if values
        values.each do |name, value|
          self[name] = value
        end
      end
    end

    # call-seq:
    #   record == other -> true/false
    #
    # _record_と_other_が同じgroongaのレコードなら+true+を返し、
    # そうでなければ+false+を返す。
    def ==(other)
      self.class == other.class and
        [table, id] == [other.table, other.id]
    end

    # call-seq:
    #   record[column_name] -> 値
    #
    # このレコードの_column_name_で指定されたカラムの値を返す。
    def [](column_name)
      @table.column_value(@id, column_name, :id => true)
    end

    # call-seq:
    #   record[column_name] = 値
    #
    # このレコードの_column_name_で指定されたカラムの値を設定す
    # る。
    def []=(column_name, value)
      @table.set_column_value(@id, column_name, value, :id => true)
    end

    # call-seq:
    #   record.append(column_name, value)
    #
    # このレコードの_column_name_で指定されたカラムの値の最後に
    # _value_を追加する。
    def append(column_name, value)
      column(column_name).append(@id, value)
    end

    # call-seq:
    #   record.prepend(column_name, value)
    #
    # このレコードの_column_name_で指定されたカラムの値の最初に
    # _value_を追加する。
    def prepend(column_name, value)
      column(column_name).prepend(@id, value)
    end

    # call-seq:
    #   record.have_column?(name) -> true/false
    #
    # 名前が_name_のカラムがレコードの所属するテーブルで定義され
    # ているなら+true+を返す。
    def have_column?(name)
      column(name).is_a?(Groonga::Column)
    rescue Groonga::NoSuchColumn
      false
    end

    # call-seq:
    #   record.reference_column?(name) -> true/false
    #
    # 名前が_name_のカラムが参照カラムであるなら+true+を返す。
    def reference_column?(name)
      column(name).range.is_a?(Groonga::Table)
    end

    # call-seq:
    #   record.search(name, query, options={}) -> Groonga::Hash
    #
    # 名前が_name_のGroonga::IndexColumnのsearchメソッドを呼ぶ。
    # _query_と_options_はそのメソッドにそのまま渡される。詳しく
    # はGroonga::IndexColumn#searchを参照。
    def search(name, query, options={})
      column(name).search(query, options)
    end

    # call-seq:
    #   record.key -> 主キー
    #
    # レコードの主キーを返す。
    #
    # _record_が所属するテーブルがGroonga:::Arrayの場合は常
    # に+nil+を返す。
    def key
      if @table.is_a?(Groonga::Array)
        nil
      else
        @key ||= @table.key(@id)
      end
    end

    # call-seq:
    #   record.score -> スコア値
    #
    # レコードのスコア値を返す。検索結果として生成されたテーブル
    # のみに定義される。
    def score
      self["._score"]
    end

    # call-seq:
    #   record.n_sub_records -> 件数
    #
    # 主キーの値が同一であったレコードの件数を返す。検索結果とし
    # て生成されたテーブルのみに定義される。
    def n_sub_records
      self["._nsubrecs"]
    end

    # call-seq:
    #   record.value -> 値
    #
    # レコードの値を返す。
    def value
      @table.value(@id, :id => true)
    end

    # call-seq:
    #   record.value = 値
    #
    # レコードの値を設定する。既存の値は上書きされる。
    def value=(value)
      @table.set_value(@id, value, :id => true)
    end

    # call-seq:
    #   record.increment!(name, delta=nil)
    #
    # このレコードの_name_で指定されたカラムの値を_delta_だけ増
    # 加する。_delta_が+nil+の場合は1増加する。
    def increment!(name, delta=nil)
      column(name).increment!(@id, delta)
    end

    # call-seq:
    #   record.decrement!(name, delta=nil)
    #
    # このレコードの_name_で指定されたカラムの値を_delta_だけ減
    # 少する。_delta_が+nil+の場合は1減少する。
    def decrement!(name, delta=nil)
      column(name).decrement!(@id, delta)
    end

    # call-seq:
    #   record.columns -> Groonga::Columnの配列
    #
    # レコードが所属するテーブルの全てのカラムを返す。
    def columns
      @table.columns
    end

    # call-seq:
    #   attributes -> Hash
    #
    # レコードが所属しているテーブルで定義されているインデックス
    # 型のカラムでない全カラムを対象とし、カラムの名前をキーとし
    # たこのレコードのカラムの値のハッシュを返す。
    def attributes
      attributes = {"id" => id}
      _key = key
      attributes["key"] = _key if _key
      table_name = @table.name
      columns.each do |column|
        next if column.is_a?(Groonga::IndexColumn)
        value = column[@id]
        # TODO: support recursive reference.
        value = value.attributes if value.is_a?(Groonga::Record)
        attributes[column.local_name] = value
      end
      attributes
    end

    # call-seq:
    #   record.delete
    #
    # レコードを削除する。
    def delete
      @table.delete(@id)
    end

    # call-seq:
    #   record.lock(options={})
    #   record.lock(options={}) {...}
    #
    # レコードが所属するテーブルをロックする。ロックに失敗した場
    # 合はGroonga::ResourceDeadlockAvoided例外が発生する。
    #
    # ブロックを指定した場合はブロックを抜けたときにunlockする。
    #
    # 利用可能なオプションは以下の通り。
    #
    # [_:timeout_]
    #   ロックを獲得できなかった場合は_:timeout_秒間ロックの獲
    #   得を試みる。_:timeout_秒以内にロックを獲得できなかった
    #   場合は例外が発生する。
    def lock(options={}, &block)
      @table.lock(options.merge(:id => @id), &block)
    end

    # call-seq:
    #   record.unlock(options={})
    #
    # レコードが所属するテーブルのロックを解除する。
    #
    # 利用可能なオプションは現在は無い。
    def unlock(options={})
      @table.unlock(options.merge(:id => @id))
    end

    # call-seq:
    #   record.clear_lock(options={})
    #
    # レコードが所属するテーブルのロックを強制的に解除する。
    #
    # 利用可能なオプションは現在は無い。
    def clear_lock(options={})
      @table.clear_lock(options.merge(:id => @id))
    end

    # call-seq:
    #   record.locked?(options={}) -> true/false
    #
    # レコードが所属するテーブルがロックされていれば+true+を返す。
    #
    # 利用可能なオプションは現在は無い。
    def locked?(options={})
      @table.locked?(options.merge(:id => @id))
    end

    def methods(include_inherited=true) # :nodoc:
      _methods = super
      return _methods unless include_inherited
      columns.each do |column|
        name = column.local_name
        _methods << name
        _methods << "#{name}="
      end
      _methods
    end

    def respond_to?(name) # :nodoc:
      super or !@table.column(name.to_s.sub(/=\z/, '')).nil?
    end

    private
    def column(name) # :nodoc:
      _column = @table.column(name.to_s)
      raise NoSuchColumn, "column(#{name.inspect}) is nil" if _column.nil?
      _column
    end

    def method_missing(name, *args, &block)
      if /=\z/ =~ name.to_s
        base_name = $PREMATCH
        is_setter = true
      else
        base_name = name.to_s
        is_setter = false
      end
      _column = @table.column(base_name)
      if _column
        if is_setter
          _column.send("[]=", @id, *args, &block)
        else
          _column.send("[]", @id, *args, &block)
        end
      else
        super
      end
    end
  end
end
