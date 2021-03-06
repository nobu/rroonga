# Copyright (C) 2009  Kouhei Sutou <kou@clear-code.com>
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

class FixSizeColumnTest < Test::Unit::TestCase
  include GroongaTestUtils

  def setup
    setup_database

    setup_bookmarks_table
  end

  def setup_bookmarks_table
    @bookmarks_path = @tables_dir + "bookmarks"
    @bookmarks = Groonga::Array.create(:name => "Bookmarks",
                                       :path => @bookmarks_path.to_s)

    @viewed_column_path = @columns_dir + "viewed"
    @viewed = @bookmarks.define_column("viewed", "Int32",
                                       :path => @viewed_column_path.to_s)
  end

  def test_inspect
    assert_equal("#<Groonga::FixSizeColumn " +
                 "id: <#{@viewed.id}>, " +
                 "name: <Bookmarks.viewed>, " +
                 "path: <#{@viewed_column_path}>, " +
                 "domain: <Bookmarks>, " +
                 "range: <Int32>, " +
                 "flags: <KEY_INT>" +
                 ">",
                 @viewed.inspect)
  end

  def test_domain
    assert_equal(@bookmarks, @viewed.domain)
  end

  def test_table
    assert_equal(@bookmarks, @viewed.table)
  end

  def test_assign_time
    comments = Groonga::Array.create(:name => "Comments")
    comments.define_column("title", "ShortText")
    comments.define_column("issued", "Time")
    title = "Good"
    issued = 1187430026
    record = comments.add(:title => title, :issued => issued)
    assert_equal([title, Time.at(issued)],
                 [record["title"], record["issued"]])
  end
end
