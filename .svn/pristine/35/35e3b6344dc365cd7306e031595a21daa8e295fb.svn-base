window.onload = function () {
    var uncompressdata = [];
    var compressdata = [];
    var encryptdata = [];
    var uncompressgraph = $("#uncompressgraph");
    var compressgraph = $("#compressgraph");
    var encryptgraph = $("#encryptgraph");

    var iteration = 0;
    
    function fetchData() {
        if (iteration < 1000) {
            function onDataReceived(series) {

                if (series.uptime == -1) {
                    $("#status").text("System is down!");
                    $("#uptime").text();
                }
                else {
                    $("#status").text("System is up!");
                    $("#uptime").text("Total uptime: " + series.uptime + " seconds");
                }

                $("#totaluncompress").text("Total uncompressed size transfered: " + series.tuncompress + " bytes");
                $("#totalcompress").text("Total compressed size transfered: " + series.tcompress + " bytes");
                $("#totalencrypt").text("Total encrypted size transfered: " + series.tencrypt + " bytes");
                $("#totalsaved").text("Saved a total of " + (series.tuncompress-series.tencrypt) + " bytes with compression and encryption.");
                var uncompresscoordinate = []
                uncompresscoordinate.push(iteration);
                uncompresscoordinate.push(series.duncompress);
                uncompressdata.push(uncompresscoordinate);

                var compresscoordinate = []
                compresscoordinate.push(iteration);
                compresscoordinate.push(series.dcompress);
                compressdata.push(compresscoordinate);

                var encryptcoordinate = []
                encryptcoordinate.push(iteration);
                encryptcoordinate.push(series.dencrypt);
                encryptdata.push(encryptcoordinate);

                iteration++;

                if (uncompressdata.length > 10) {
                    uncompressdata.shift();
                }

                if (compressdata.length > 10) {
                    compressdata.shift();
                }

                if (encryptdata.length > 10) {
                    encryptdata.shift();
                }
                
                var minx = Math.max(0, iteration-10);
                var maxx = minx+9;
                $.plot(uncompressgraph, [uncompressdata], {xaxis: {min: minx, max: maxx}});
                $.plot(compressgraph, [compressdata], {xaxis: {min: minx, max: maxx}});
                $.plot(encryptgraph, [encryptdata], {xaxis: {min: minx, max: maxx}});
                
                
                
        }
            $.ajax({
                url: "/data",
                method: 'GET',
                dataType: 'json',
                success: onDataReceived
            });
        }
        else {
            clearInterval(timer);
        }
    }
    var timer = setInterval(fetchData, 5000);


};