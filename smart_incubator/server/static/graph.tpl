<!--
   index.html
   
   Copyright 2016 Giorgio Gilestro <giorgio@gilest.ro>
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
   MA 02110-1301, USA.
   
   
-->

<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN"
    "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">
<html xmlns="http://www.w3.org/1999/xhtml" xml:lang="en" lang="en">

<head>
    <title>Incubators Dashboard</title>
    <meta http-equiv="content-type" content="text/html;charset=utf-8" />
    <meta name="generator" content="Geany 1.28" />
    
    <link rel="stylesheet" type="text/css" href="/static/css/style.css">
    <link rel="stylesheet" type="text/css" href="/static/css/style-graphs.css" media="all" /> 
    <link rel="stylesheet" href="/static/css/morris.css">

    <link href='//fonts.googleapis.com/css?family=Varela+Round' rel='stylesheet' type='text/css'>

    <script type="text/javascript" src="/static/js/jquery.min.js"></script>
    <script type="text/javascript" src="/static/js/moment.js"></script>
    <script type="text/javascript" src="/static/js/script.js"></script>

    <!-- charts -->
    <script src="/static/js/modernizr.js"></script>
    <script src="/static/js/raphael-min.js"></script>
    <script src="/static/js/morris.js"></script>
    <!-- //charts -->

    <script type="text/javascript">
        
    $(document).ready(function(){ 
            loadGraphs({{incubator_id}});
            loadIncubatorForm({{incubator_id}});
            show_alert_box();
        });  
        
    </script>

    
</head>

<body>
<header>
    <nav class="navbar">
    <div class="container">
        <div class="navbar-header">
            <a class="navbar-brand" href="/">Incubators</a>
        </div>

        <ul class="nav navbar-nav navbar-right">
            <li><a href="localhost/" target="_blank">Ethoscopes</a></li>
        </ul>
    </div>
    </nav>
</header>


<div class="main w3l">
        
    <div class="alert" id="alert">
      <span class="closebtn" onclick="this.parentElement.style.display='none';">&times;</span> 
      {{message}}
    </div>
    
    <div class="graph lost">
        <div class="agile">
            <div class="w3l-agile">
                <h3>Incubator {{incubator_id}}</h3>
            </div>
            <div id="temperature"></div>
            <div id="humidity"></div>
            <div id="light"></div>
        </div>
    </div>

    <div class="agile">
        <div id="incubator-data"></div>
    </div>


</div>

<footer></footer>

</body>
</html>
