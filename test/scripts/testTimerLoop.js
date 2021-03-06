print("Creating Loop");

// create a loop
var loop = new QDaqLoop("loop");
loop.period = 200;
// create a loop
var loop2 = new QDaqLoop("loop2");
loop2.delay = 5;
// create a clock channel
var t = new QDaqChannel("t");
t.type = "Clock";
// create a clock channel
var t2 = new QDaqChannel("t2");
t2.type = "Clock";
// create a test random channel
var ch1 =  new QDaqChannel("ch1");
ch1.type = "Random";
// create a 2nd channel
var ch2 = new QDaqChannel("ch2");
// create a script job
// that will write to ch2 the square root of ch1
var scr = new QDaqJob("scr");
scr.code = "var v = qdaq.loop.ch1.value(); qdaq.loop.loop2.ch2.push(Math.sqrt(v));"
// build the object hierarchy under the root object "qdaq"
loop.appendChild(t);
loop.appendChild(ch1);
loop2.appendChild(t2);
loop2.appendChild(scr);
loop2.appendChild(ch2);
loop.appendChild(loop2);
qdaq.appendChild(loop);

loop.createLoopEngine();

print("Tree = \n" + qdaq.objectTree());

var w = loadTopLevelUi('ui/testTimerForm.ui','mainForm');
bind(qdaq.loop.t,w.findChild('t'));
bind(qdaq.loop.loop2.t2,w.findChild('t2'));
bind(qdaq.loop.ch1,w.findChild('ch1'));
bind(qdaq.loop.loop2.ch2,w.findChild('ch2'));

function startPressed(on) {
    if (on) qdaq.loop.arm();
    else qdaq.loop.disarm();
}

function startPressed2(on) {
    if (on) qdaq.loop.loop2.arm();
    else qdaq.loop.loop2.disarm();
}

startButton = w.findChild("btStart");
startButton.toggled.connect(startPressed);

startButton = w.findChild("btStart2");
startButton.toggled.connect(startPressed2);

w.show()

