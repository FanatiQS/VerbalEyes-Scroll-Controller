// Show hides configuration items on click
document.querySelectorAll('.config-container').forEach((node) => {
	node.onclick = function (event) {
		if (event.target === this.lastElementChild) return;
		this.classList.toggle("config-container-open");
	};
});

// Function to convert form to a configuration string
function serializeConfig() {
	const textboxes = document.querySelectorAll(".config-container-open .config-textbox");
	let output = '';
	for (const textbox of textboxes) {
		output += `${textbox.name}=${textbox.value}\n`;
	}
	output += '\n';
	return output;
}



// Logs a message to the on-screen TTY
const tty = document.querySelector('.webserial-console');
function webserialLog(chunk) {
	const atBottom = (tty.parentNode.scrollTop >= tty.scrollHeight - tty.parentNode.clientHeight);
	tty.textContent += chunk;
	if (atBottom) tty.parentNode.scrollTop = tty.scrollHeight;
}

// Clears log
document.querySelector('.webserial-console-clear').onclick = function () {
	tty.textContent = '';
};

// Logs error if WebSerial is not available
if (!navigator.serial) {
	webserialLog("WebSerial API not available\n");
	document.querySelector("#webserial-read").disabled = true;
	document.querySelector("#webserial-upload").disabled = true;
}



// Connects to WebSerial device if not connected
let serialDevice = null;
async function connectWebSerial() {
	if (serialDevice) return;
	serialDevice = await new SerialDevice();
	document.querySelector('#webserial-disconnect').disabled = false;
	serialDevice.reader.read(); // Flushes read buffer to fix half messages printed
}

// Starts reading data from serial until the callback is no longer defined
let serialReadCallback = null;
async function readSerialWithCallback(callback) {
	if (serialReadCallback) return;
	serialReadCallback = callback;
	for await (const chunk of serialDevice) {
		if (!serialReadCallback) return;
		serialReadCallback(chunk);
	}
}

// Reads logs from web-serial to console
document.querySelector("#webserial-read").onchange = async function () {
	// Connects serial device if not connected already
	await connectWebSerial();

	// Add callback to serial read if checkbox is checked
	if (this.checked) {
		readSerialWithCallback((chunk) => {
			const index = chunk.indexOf('\r\n');
			if (index === -1) return;
			webserialLog(chunk.slice(index + 2));
			serialReadCallback = webserialLog;
		});
	}
	// Removes callback if this one is reading and wants to stop
	else if (serialReadCallback === webserialLog) {
		serialReadCallback = null;
	}
};

// Uploads configuration over web-serial
document.querySelector('#webserial-upload').onclick = async function () {
	if (!document.querySelectorAll(".config-container-open").length) {
		return;
	}

	// Connects to device and sends serialized data
	await connectWebSerial();
	serialDevice.write(serializeConfig());

	// Do not read if data is not already being read
	if (document.querySelector("#webserial-read").checked) return;

	// Reads response data up to configuration is done
	let trimStartDone = false;
	let buffer = '';
	readSerialWithCallback((chunk) => {
		// Buffers incomplete lines until next chunk and creates string without incomplete
		const splitted = (buffer + chunk).split("\r\n");
		buffer = splitted.pop();
		const msg = splitted.join('\r\n');

		// Exits after configuration ends
		if (msg.indexOf("Configuration saved\r\n") > -1 || msg.indexOf("Configuration canceled\r\n") > -1) {
			webserialLog(msg.slice(0, msg.indexOf('\n')) + '\n');
			serialReadCallback = null;
		}
		// Trims out everything before configuration starts
		else if (!trimStartDone) {
			const index = msg.indexOf('[');
			if (index >= 0) {
				webserialLog(msg.slice(index) + '\n');
				trimStartDone = true;
			}
		}
		// Prints logs
		else {
			webserialLog(msg + '\n');
		}
	});
};

// Disconnects curret web-serial device
document.querySelector('#webserial-disconnect').onclick = function () {
	serialDevice.close();
	serialDevice = null;
	this.disabled = true;
	document.querySelector("#webserial-read").checked = false;
	serialReadCallback = null;
};



// Loads local file to config
document.querySelector('#config-load').onclick = function () {
	const loadHandler = document.createElement('input');
	loadHandler.setAttribute('type', 'file');
	loadHandler.onchange = function () {
		reader.readAsText(this.files[0]);
	};
	const reader = new FileReader();
	reader.onload = function (event) {
		event.target.result.split('\n').forEach((line) => {
			const [ key, value ] = line.split('=');
			if (!key) return;
			document.querySelector(`input[name=${key}]`).value = value;
		});
	};
	loadHandler.click();
};

// Save config to local file
document.querySelector('#config-save').onclick = function (event) {
	const saveHandler = document.createElement('a');
	saveHandler.setAttribute('download', 'config.txt');
	if (!confirm("Do you want to download your current configuration?")) return;
	saveHandler.setAttribute('href', `data:text/plain,${encodeURIComponent(serializeConfig('\n'))}`);
	saveHandler.click();
};



// // Sets default OS for manual configuration
// document.querySelector('#generate-os').selectIndex = navigator.platform.startsWith('Win') * 1 || !navigator.platform.startsWith('Mac') * 2;
//
// // Generates manual configuration command
// document.querySelector("#gen-conf").onclick = function () {
// 	const os = this.previousElementSibling.selectedOptions[0].value;
// 	let output = "Copy and paste this command into you command line tool:\n";
// 	switch (os) {
// 		case "macos": {
// 			output += `printf '${serializeConfig().replace(/\n/g, '\\n')}' > \`ls /dev/cu.usbserial-* | head -1\`; cat < \`ls /dev/cu.usbserial-* | head -1\``;
// 			break;
// 		}
// 		case "linux": {
// 			output += `printf '${serializeConfig().replace(/\n/g, '\\n')}' > \`ls /dev/ttyUSB* | head -1\`; cat < \`ls /dev/ttyUSB* | head -1\``;
// 			break;
// 		}
// 		case "windows": {
// 			log("\nWindows is currently not supported\n");
// 			return;
// 		}
// 	}
// 	log(output + '\n');
// };
