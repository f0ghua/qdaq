
// create a loop
var loop = new QDaqLoop("loop");
// create a clock channel
var t = new QDaqChannel("t");
t.type = "Clock";
// create the control signal and sys. output channels
var u =  new QDaqChannel("u");
var y =  new QDaqChannel("y");
var y1 =  new QDaqChannel("y1");
// create sys
var sys = new QDaqFilter("sys");
sys.loadPlugin("libqdaqfopdt.so");
sys.inputChannels = [u];
sys.outputChannels = [y];
// create PID
var pid = new QDaqFilter("pid");
pid.loadPlugin("libqdaqpid.so");
pid.inputChannels = [y];
pid.outputChannels = [u];
// create interp
var ip = new QDaqFilter("ip");
ip.loadPlugin("libqdaqinterpolator.so");
ip.inputChannels = [y];
ip.outputChannels = [y1];
ip.interpolator.type = "Linear";
ip.interpolator.fromTextFile("scripts/tbl.dat");




// build the object hierarchy under the root object "qdaq"
loop.appendChild(t);
loop.appendChild(u);
loop.appendChild(sys);
loop.appendChild(y);
loop.appendChild(pid);
loop.appendChild(ip);
loop.appendChild(y1);
qdaq.appendChild(loop);



loop.createLoopEngine();
loop.period = 100;

u.push(1.);

print("Tree = \n" + qdaq.objectTree());

var w = loadTopLevelUi('ui/testPID.ui','mainForm');
bind(qdaq.loop.t,w.findChild('t'));
bind(qdaq.loop.u,w.findChild('u'));
bind(qdaq.loop.y,w.findChild('y'));
bind(qdaq.loop.y1,w.findChild('y1'));

function startPressed(on) {
    if (on) qdaq.loop.arm();
    else qdaq.loop.disarm();
}

startButton = w.findChild("btStart");
startButton.toggled.connect(startPressed);

w.show()
