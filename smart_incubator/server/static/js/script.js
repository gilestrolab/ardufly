function loadDashboard(){
     $.getJSON("/json/all", 
          function(data) {
            $.each(data, function(i, item) {
                
                is_timestamp = new Date(item.device_time*1000).getTime() > 0;
                if (is_timestamp) { 
                    var timestamp = item.device_time;
                } else {
                    var k = moment(item.device_time);
                    var timestamp = (k + k.utcOffset()*60*1000)/1000;
                }
               var time = moment(timestamp*1000).format("DD-MM-YYYY HH:mm");                
                
                $('#main').append('\
                    <div class="incubator" id="'+item.inc_id+'">\
                        <h2><a href="/graph/'+item.inc_id+'/1">Incubator '+item.inc_id+'</a></h2>\
                        <div class="data">\
                            <p class="temperature">'+item.temperature+'</p><div class="light"></div></div>\
                        <div class="data">\
                            <p class="humidity">'+item.humidity+'</p></div>\
                        <div class="time"><p>'+time+'</p></div>\
                        </div>');
            })
          }); 
  }

function loadIncubatorForm(incubator_id){
     $.getJSON("/json/" + incubator_id, 
          function(data) {

                is_timestamp = new Date(data.device_time*1000).getTime() > 0;
                if (is_timestamp) { 
                    var timestamp = data.device_time;
                } else {
                    var k = moment(data.device_time);
                    var timestamp = (k + k.utcOffset()*60*1000)/1000;
                }
                var time = moment(timestamp*1000).format("DD-MM-YYYY HH:mm");                

                $('#incubator-data').append('\
                            <form class="incubator-form" method="post">\
                            <p>Incubator '+data.inc_id+' - Last reading: '+time+'</p>\
                            <p><label>Current temperature is: '+data.temperature+' / </label><input type="text" name="set_temp" value="'+data.set_temp+'"  size="2"></p>\
                            <p><label>Current humidity is: '+data.humidity+' / </label><input type="text" name="set_hum" value="'+data.set_hum+'"  size="2"></p>\
                            <p><label>Current Light is:  '+data.light/10+'. Light power is set at:  </label><input type="text" name="set_light" value="'+data.set_light+'"  size="2"></p>\
                            <p><label>Light Regime </label>\
                            <select name="dd_mode" >\
                                <option value="0">DD</option>\
                                <option value="1">LD</option>\
                                <option value="2">LL</option>\
                                <option value="3">DL</option>\
                                <option value="4">MM</option>\
                            </select></p>\
                            <p><label>Lights ON at: </label><input type="text" name="lights_on" value="'+data.lights_on+'" size="3"></p>\
                            <p><label>Lights OFF at: </label><input type="text" name="lights_off" value="'+data.lights_off+'" size="3"></p>\
                            <input type="hidden" name="submitted" value="1">\
                            <input type="submit" value="Change" class="submit" enctype="multipart/form-data">\
                            </form>\
                        </div>');
                $('.incubator-form option[value='+data.dd_mode+']').attr('selected','selected');
          }); 
  }


function refreshDashboard(){
     $.getJSON("/json/all", 
          function(data) {
            $.each(data, function(i, item) {
                
                is_timestamp = new Date(item.device_time*1000).getTime() > 0;
                
                if (is_timestamp) { 
                    var timestamp = item.device_time;
                } else {
                    var k = moment(item.device_time);
                    var timestamp = (k + k.utcOffset()*60*1000)/1000;
                }

               var time = moment(timestamp*1000).format("DD-MM-YYYY HH:mm");
               var temperature = item.temperature;
                
                if (Math.abs(item.temperature - item.set_temp ) > 0.5) 
                    { $('#' + item.inc_id).find('.temperature').css('color','#934c4c'); }
                else
                    { $('#' + item.inc_id).find('.temperature').css('color','#777'); }
                if (Math.abs(item.humidity - item.set_hum ) > 2) 
                    { $('#' + item.inc_id).find('.humidity').css('color','#934c4c'); }
                else
                    { $('#' + item.inc_id).find('.humidity').css('color','#777'); }
                
            $('#' + item.inc_id).find('.temperature').html(item.temperature);
            $('#' + item.inc_id).find('.temperature').attr('title', item.temperature + " / " + item.set_temp);
            
            $('#' + item.inc_id).find('.light').css('opacity', item.light/100.0);
            $('#' + item.inc_id).find('.light').attr('title', item.light + " / " + item.set_light*10);
            
            $('#' + item.inc_id).find('.humidity').html(item.humidity);
            $('#' + item.inc_id).find('.humidity').attr('title', item.humidity + " / " + item.set_hum);
            
            $('#' + item.inc_id).find('.time').find("p").html(item.device_time);
            $('#' + item.inc_id).find('.time').attr('title', timestamp);

            })
          }); 
     window.setTimeout(refreshDashboard,1*60*1000); //we refresh once a minute
  }

function refreshSerialMonitor(){
    
      updateClock();
      $.getJSON(
            "/json/serial",
            function (data) { 
                  $('#serialoutput').val(data.result);
            }
        );            
      window.setTimeout(refreshSerialMonitor,1000);
}

function updateClock() {
    $('.current-time').text(moment().utc());
}

function checkTimeValues(){
    

    var now = moment().utc();
    updateClock();
    
    $('.time').each( function (i) {
        
        var timestamp = $(this).attr('title');
        var time = moment( timestamp * 1000);
        var delta = now - time; // in ms

        alert_range = 15 * 60 * 1000; //15minutes
        
        if (delta >= alert_range) { 
            $(this).find('p').css('color','#934c4c');
            $(this).find('p').css('font-weight','bold');
        } else {
            $(this).find('p').css('color','#777');
            $(this).find('p').css('font-weight','normal');
        }
    })
    window.setTimeout(checkTimeValues,1000)
}

function show_alert_box() {

    if ($("#alert").text().length > 34)  { $('#alert').show() }
    else { $('#alert').hide() }
    
}

function show_time_buttons(incubator_id, day) {
 
    $("#time-buttons").html('<a id=\"previous-day\" href="/graph/'+incubator_id+'/'+(day+1)+'">Previous Day</a> - <a id=\"next-day\" href="/graph/'+incubator_id+'/'+(day-1)+'">Next Day</a>');
     
}

function loadGraphs(incubator_id, days){
        
        dateFrom = moment().subtract(days,'d').format('YYYY MMM DD dddd');
        
        $('#graph-title').text( $('#graph-title').text() + " - " + dateFrom )
        
        $.ajax({
            url: "/incubator/"+incubator_id+"/"+days,
            dataType: 'JSON',
            data: {get_values: true},
            success: function(response) {
                Morris.Line({
                    element: 'temperature',
                    data: response,
                    xkey: 'device_time',
                    ykeys: ['temperature'],
                    labels: ['Temperature'],
                    ymax: '30',
                    ymin: '18'
                });
                
                Morris.Line({
                    element: 'humidity',
                    data: response,
                    xkey: 'device_time',
                    ykeys: ['humidity', 'light'],
                    labels: ['Humidity', 'Light'],
                    ymax: '100',
                    ymin: '0'

                });
                
            }
        });
    }
