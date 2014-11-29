class SiteController < ApplicationController
	respond_to :html, :js
  def index
    @stat = Stat.first
    @linenum = 1
    f = File.open(File.expand_path('~/.DartSync/.statslog'), "r")
    f.each_line { |line| 
      if @linenum == 1
        @stat.duncompress = Integer(line)
      end
      if @linenum == 2
        @stat.tuncompress = Integer(line)
      end
      if @linenum == 3
        @stat.dcompress = Integer(line)
      end
      if @linenum == 4
        @stat.tcompress = Integer(line)
      end
      if @linenum == 5
        @stat.dencrypt = Integer(line)
      end
      if @linenum == 6
        @stat.tencrypt = Integer(line)
      end
      if @linenum == 7
        @stat.starttime = Integer(line.to_i)
      end
      if @linenum == 8
        @stat.updatetime= Integer(line.to_i)
      end
      @linenum += 1
    } 
    f.close

    @up = false;
    if Time.now.getutc.to_i - @stat.updatetime.to_i < 30
      @uptime = (Time.now.getutc.to_i-@stat.starttime.to_i).round
      @up = true
    end

    @stat.save


  end

  def about
  end
  def contact
  end
  def data
    @stat = Stat.first
    @linenum = 1
    f = File.open(File.expand_path('~/.DartSync/.statslog'), "r")
    f.each_line { |line| 
      if @linenum == 1
        @stat.duncompress = Integer(line)
      end
      if @linenum == 2
        @stat.tuncompress = Integer(line)
      end
      if @linenum == 3
        @stat.dcompress = Integer(line)
      end
      if @linenum == 4
        @stat.tcompress = Integer(line)
      end
      if @linenum == 5
        @stat.dencrypt = Integer(line)
      end
      if @linenum == 6
        @stat.tencrypt = Integer(line)
      end
      if @linenum == 7
        @stat.starttime = Integer(line.to_i)
      end
      if @linenum == 8
        @stat.updatetime= Integer(line.to_i)
      end
      @linenum += 1
    } 
    f.close
    
    @uptime = -1
    if (Time.now.getutc.to_i - @stat.updatetime < 30)
      @uptime = (Time.now.utc.getutc.to_i - @stat.starttime.to_i)
    end
    
    @stat.save
    render(:partial => "data", :format => [:json])

  end
end
