<?php
error_reporting(E_ALL);

$dir = '/home/captcha/web/img';
$filename = '';
while ($filename == '' || is_file($dir.$filename)) {
	$filename = random_string(10);
}
$filename .= ".png";

$data = shell_exec("../TeeCaptcha -x 5 -X 2 -y 6 -Y 3 -g ".escapeshellarg("./img/".$filename));
list($w, $h, $target) = explode(' ', $data);
$targetX = $target % $w;
$targetY = (int)($target / $w);

/* generates a random string [0-9a-zA-Z] */
function random_string($length, $base = 62) {
	$str = '';
	while ($length--) {
		$x = mt_rand(1, min($base, 62));
		$str .= chr($x + ($x > 10 ? $x > 36 ? 28 : 86 : 47));
	}
	return $str;
}

?>
<html>
	<head>
		<meta charset="utf-8">
		<title>TeeCaptcha</title>
		<style type="text/css">
		<!--
		body {
			font-family: Tahoma;
			background-color: #62492D;
			color: white;
		}
		a {
			color: #ddd;
		}
		#task {
		}
		#content {
			text-align: center;
		}
		img {
			border-style: solid;
			border-width: 32;
			border-color: #62492D;
			border-image: url('border1.png') 32 repeat;
		}
		#timer {
			position: absolute;
			top: 40px;
			left: 50%;
			width: 400px;
			margin-left: -<?php echo $w * 28 + 442; ?>px;
			text-align: end;
			font-size: 26px;
			font-family: monospace;
			transition: all 0.3s;
		}
		#misses {
			position: absolute;
			top: 140px;
			left: 50%;
			width: 400px;
			margin-left: -<?php echo $w * 28 + 442; ?>px;
			text-align: end;
			font-size: 26px;
		}
		#next {
			display: none;
			position: absolute;
			top: 40px;
			left: 50%;
			height: <?php echo $h * 56; ?>px;
			margin-left: <?php echo $w * 29 + 42; ?>px;
			vertical-align: middle;
			font-size: 26px;
			background-color: #456B16;
			padding: 0 10px;
		}
		#next p {
			line-height: <?php echo $h * 56; ?>px;
			margin: 0;
		}
		-->
		</style>
		<script type="text/javascript">
		<!--
		var targetX = <?php echo $targetX; ?>,
			targetY = <?php echo $targetY; ?>;
		var starttime = 0;
		var interval;
		var backgroundColor;
		var misses = 0;
		function guess(x, y) {
			if (x == targetX && y == targetY) {
				stopTimer();
				timer();
				document.body.style.backgroundColor = "#68A421";
				document.querySelector('#timer').style.fontWeight = 'bold';
				document.querySelector('#timer').style.fontSize = '52px';
				// setTimeout("newCaptcha()", 2000);
				document.querySelector('#next').style.display = 'block';
			} else {
				var m = ++misses;
				var text = "";
				for (; m >= 5; m -= 5) {
					text += " ᚏ";
				}
				if (m == 4) text += " ᚎ";
				if (m == 3) text += " ᚍ";
				if (m == 2) text += " ᚌ";
				if (m == 1) text += " ᚋ";
				document.querySelector('#misses p').innerHTML = text;
				document.body.style.backgroundColor = "#C64305";
				setTimeout("resetBackground()", 100);
			}
		}
		function start() {
			backgroundColor = document.body.style.backgroundColor;
			interval = setInterval("timer()", 30);
			starttime = new Date().getTime();
		}
		function stopTimer() {
			clearInterval(interval);
		}
		function timer() {
			var time = (new Date().getTime() - starttime) / 1000;
			document.querySelector('#timer p').innerHTML = time.toFixed(2) + ' s';
		}
		function newCaptcha() {
			document.location.reload();
		}
		function resetBackground() {
			document.body.style.backgroundColor = backgroundColor;
		}
		function next() {
			newCaptcha();
		}
		-->
		</script>
	</head>
	<body onload="start()">
		
		<div id="content">
			
			<map name="captcha" id="captcha">
			<?php
			for ($y = 0; $y < $h; $y++) {
				for ($x = 0; $x < $w; $x++) {
					echo '<area shape="rect" coords="'.implode(',', array($x*56, $y*56, $x*56+56, $y*56+56)).''.'" href="javascript:guess('.$x.','.$y.');" alt="">';
				}
			}
			?>
			</map>
			<img src="/img/<?php echo $filename; ?>" alt="" usemap="#captcha">
			
			<p id="task">Find the tee with no eyes.</p>
			
			<br>

			<p>This is a playful demo of my own captcha generator. The generator itself is written in C and uses images from <a href="https://www.teeworlds.com/">Teeworlds</a>. The frontend is PHP. I'm planning to use these captchas on my websites and I'm also thinking about making it a captcha service so other websites can use them, too.</p>
			
			<p>Impress: Marius Neugebauer &bull; Steinkaulstr. 52 &bull; 52070 Aachen &bull; Germany &bull; +49 (0) 1578 738 9019 &bull; teevision@teevision.eu</p>
			
		</div>
		
		<div id="timer">
			<p>0.00 s</p>
		</div>
		
		<div id="misses">
			<p></p>
		</div>
		
		<div id="next" onmouseover="next()">
			<p>next &gt;</p>
		</div>
		
	</body>
</html>