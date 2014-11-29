class Associations < ActiveRecord::Migration
  def change
  	create_table :stats do |t|
  		t.integer :deltacompressed
  		t.integer :deltadecompressed
  		t.integer :deltatotalsize
  		t.integer :deltafilesupdated
      t.integer :totalcompressed
      t.integer :totaldecompressed
      t.integer :totaltotalsize
      t.integer :totalfilesupdated
  		t.integer :numpeers
  		t.datetime :start
  		t.datetime :updated
  		t.timestamps
  	end
  end
end
