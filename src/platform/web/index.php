<!DOCTYPE html>
<html>
		<head>
			<title>OpenLara</title>
			<style>
			html {
				overflow: hidden;
			}
			body {
				margin: 0px;
				font-size: 1.0em;
				overflow: hidden;
			}
			.game_fs {
				position: fixed;
				top: 0px;
				left: 0px;
				margin: 0px;
				width: 100%;
				height: 100%;
				border: 0;
				display: block;
			}
			table {
				border-collapse: collapse;
			}

			table, th, td {
				border: 1px solid black;
				padding: 4px;
			}
			</style>
			<meta charset="utf-8">
			<meta name="apple-mobile-web-app-capable" content="yes">
			<meta name="viewport" content="width=854, user-scalable=no">
		</head>
		<body>
			<button id="goFS">Go fullscreen</button>
			<canvas class="game" id="canvas" width="854" height="480" oncontextmenu="event.preventDefault()"></canvas><br>
			<div id="status">Starting...</div>
			<script type='text/javascript'>
				var statusElement = document.getElementById('status');
				var canvasElement = document.getElementById('canvas');
				var proc;

				function fullscreen(container) {
					Module.ccall('change_fs_mode', 'null', [], []);
					canvasElement.focus();
				}

				var goFS = document.getElementById("goFS");
				document.getElementById("goFS").style.display = 'block';
				goFS.addEventListener("click", function() { fullscreen(canvasElement); }, false);

				window.onload = function() { window.focus(); }
				
				var Module = {
					TOTAL_MEMORY: 64*1024*1024,
					preRun: [],
					postRun: [],
					print: (function() {
								return function(text) {
									if (arguments.length > 1) text = Array.prototype.slice.call(arguments).join(' ');
									console.log(text);
								};
							})(),
					printErr: function(text) {
						if (arguments.length > 1) text = Array.prototype.slice.call(arguments).join(' ');
						if (0) {
							dump(text + '\n');
						} else {
							console.error(text);
						}
					},
					canvas: (function() {
						canvasElement.addEventListener("webglcontextlost", function(e) { alert('WebGL context lost. You will need to reload the page.'); e.preventDefault(); }, false);
						return canvasElement;
					})(),
					setStatus: function(text) {
						if (!Module.setStatus.last) Module.setStatus.last = { time: Date.now(), text: '' };
						if (text === Module.setStatus.text) return;
						statusElement.innerHTML = text;
					},
					totalDependencies: 0,
					monitorRunDependencies: function(left) {
						this.totalDependencies = Math.max(this.totalDependencies, left);
						Module.setStatus(left ? 'Preparing... (' + (this.totalDependencies-left) + '/' + this.totalDependencies + ')' : 'All downloads complete.');
					},
				};

				var audioContext = null;

				function snd_init() {
					var AudioContext = window.AudioContext || window.webkitAudioContext;
					if (!AudioContext) return;
					audioContext = new (window.AudioContext || window.webkitAudioContext)();
					var count  = 4096;
					var rate   = 44100 / audioContext.sampleRate;
					var framesCount = Math.ceil(count * rate);
					var frames = Module._malloc(framesCount * 4); // interleaved short L, R
					proc = audioContext.createScriptProcessor(count, 2, 2);
					proc.onaudioprocess = function(e) {
						var L = e.outputBuffer.getChannelData(0),
							R = e.outputBuffer.getChannelData(1);
						Module.ccall('snd_fill', 'null', ['number', 'number'], [frames, framesCount]);
						for (var i = 0; i < count; i++) {
							var index = frames + Math.floor(i * rate) * 4; // 4 is sample frame stride in bytes
							L[i] = Module.getValue(index    , 'i16') / 0x8000;
							R[i] = Module.getValue(index + 2, 'i16') / 0x8000;
						}
					}
					proc.connect(audioContext.destination);
				}

				function getLanguage() {
					var lang = navigator.languages && navigator.languages[0] || navigator.language || navigator.userLanguage;
					if (!lang) return;
					var id = 0;
					lang = lang.substring(0, 2);
					if (lang == "fr") {
						id = 1;
					} else if (lang == "de") {
						id = 2;
					} else if (lang == "es") {
						id = 3;
					} else if (lang == "it") {
						id = 4;
					} else if (lang == "pl") {
						id = 5;
					} else if (lang == "pt") {
						id = 6;
					} else if (lang == "ru" || lang == "be" || lang == "uk") {
						id = 7;
					} else if (lang == "ja") {
						id = 8;
					}
					Module.ccall('set_def_lang', 'null', ['number'], [id]);
				}

			// unlock audio context after user interaction
				var userAction = function() {
					if (audioContext && audioContext.state == "suspended") {
						audioContext.resume().then(function() {
							window.removeEventListener('touchstart', userAction, false);
							window.removeEventListener('click', userAction, false);
							console.log("resume playback");
						});
					}
				};
				window.addEventListener('touchstart', userAction, false);
				window.addEventListener('click', userAction, false);

				Module.setStatus('Downloading...');
				window.onerror = function(event) {
					Module.setStatus('Exception thrown, see JavaScript console');
					//spinnerElement.style.display = 'none';
					document.getElementById('info').innerHTML = event;
					Module.setStatus = function(text) {
						if (text) Module.printErr('[post-exception status] ' + text);
					};
				};
			</script>

			<span id="info">
				<input type="file" id="browseFile" style="display:none" accept=".phd,.psx, .tr2" onchange="readLevel(event)" />
				<input type="button" value="Browse Level" onclick="document.getElementById('browseFile').click();" /> (.PHD, .PSX, .TR2)&nbsp;
				<!--
				<input type="button" value="Backup Saves" onclick="backupSaves();" />
				<input type="button" value="Restore Saves" onclick="restoreSaves();" />
-->
				<p style="margin:8px">
					OpenLara on <a target="_blank" href="https://github.com/XProger/OpenLara">github</a> & <a target="_blank" href="https://www.facebook.com/OpenLaraTR">facebook</a><br>
				
					<div id="latest_changes" style="margin:8px">
						<br><i>last update: <?php
							echo date("d.m.Y", filemtime("./OpenLara.js.gz"));
						?></i><br>
						<input type="button" value="Latest changes" onclick="getLatestChanges()" />
					</div>
				</p>
			</span>

			<script>
				var script = document.createElement('script');

				if ('WebAssembly' in window) {
					console.log("Start WebAssembly!");
					script.src = "OpenLara_wasm.js";
				} else {
					console.log("Start asm.js!");
					script.src = "OpenLara.js";
					(function() {
						var memoryInitializer = 'OpenLara.js.mem';
						if (typeof Module['locateFile'] === 'function') {
							memoryInitializer = Module['locateFile'](memoryInitializer);
						} else if (Module['memoryInitializerPrefixURL']) {
							memoryInitializer = Module['memoryInitializerPrefixURL'] + memoryInitializer;
						}
						var xhr = Module['memoryInitializerRequest'] = new XMLHttpRequest();
						xhr.open('GET', memoryInitializer, true);
						xhr.responseType = 'arraybuffer';
						xhr.send(null);
					})();
				}

				document.body.appendChild(script);

				//window.onbeforeunload = function () { // Ctrl+W
				//	return "Really want to quit the game?";
				//};

				var isMobile = !!navigator.platform && /iPad|iPhone|iPod/.test(navigator.platform);

				if (isMobile) {
					canvasElement.className = "game_fs";
					statusElement.style.display = 'none';
					document.getElementById('info').style.display = 'none';
				}

				function getLatestChanges() {
					var xhr = new XMLHttpRequest();
					
					xhr.onreadystatechange = function() {
						if (this.readyState === 4 && this.status === 200) {
							e = document.getElementById('latest_changes');
							str = '<table>';
							for (var i = 0; i < this.response.length; i++) {
								var item = this.response[i];
								var d = new Date(item.commit.author.date);
								var date = d.getDate() + '.' + (d.getMonth() + 1) + '.' + d.getFullYear();
								str += '<tr><td>' + date + '</td><td border="1px">' + item.commit.message + '</td></tr>';
							}
							str += '</table>';
							e.innerHTML = str;
						}
					};
					
					xhr.open('GET', "https://api.github.com/repos/XProger/OpenLara/commits", true);
					xhr.responseType = 'json';
					xhr.send(null);				
				}
				
				function readLevel(event) {
					var reader = new FileReader();
					reader.onload = function(){
						var size = reader.result.byteLength;
						var data = Module._malloc(size);
						Module.writeArrayToMemory(new Uint8Array(reader.result), data);
						Module.ccall('game_level_load', 'null', ['number', 'number'], [data, size]);
					};
					reader.readAsArrayBuffer(event.target.files[0]);
				}
				/*
				var db;      
				var request = indexedDB.open("db");
request.onupgradeneeded = function() {
console.log("!!!!!!!! NO DB!");
};
request.onsuccess = function() {
  db = request.result;
  // Enable our buttons once the IndexedDB instance is available.

};

 function logTimestamps(timestamps) {
  console.log('There are ' + timestamps.length +
	' timestamp(s) saved in IndexedDB: ' + timestamps.join(', '));
} 
				
				function backupSaves() {

  var transaction = db.transaction("FILE_DATA", 'readonly');
  var store = transaction.objectStore("FILE_DATA");
  store.get("level/1/GYMLOAD.PNG").onsuccess = function(e) {
	var value = e.target.result;
	// value will be: { zip: {}, foo: { bar: { baz: 1 } } }
	console.log(value);
					var data = value;
					var a = document.createElement("a");
					
					a.href     = URL.createObjectURL(new Blob([data]));
					a.download = "savegame.dat";
					document.body.appendChild(a);
					a.click();
					document.body.removeChild(a);    
  };

				}
*/
				function restoreSaves() {
					alert('restore');
				}
			</script>

			<script>(function(i,s,o,g,r,a,m){i['GoogleAnalyticsObject']=r;i[r]=i[r]||function(){(i[r].q=i[r].q||[]).push(arguments)},i[r].l=1*new Date();a=s.createElement(o),m=s.getElementsByTagName(o)[0];a.async=1;a.src=g;m.parentNode.insertBefore(a,m)})(window,document,'script','//www.google-analytics.com/analytics.js','ga');ga('create', 'UA-60009035-1', 'auto');ga('send', 'pageview');</script>
		
		</body>
</html>