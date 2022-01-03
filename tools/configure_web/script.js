// Show hides configuration items on click
document.querySelectorAll('.config-container').forEach((node) => {
	node.onclick = function (event) {
		if (event.target === this.lastElementChild) return;
		this.classList.toggle("config-container-open");
	};
});



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

// Creates serial device
const serialDevice = new SerialDevice(webserialLog);

// Function to convert form to a configuration string
function serializeConfig() {
	const textboxes = document.querySelectorAll(".config-container-open .config-textbox");
	return serialDevice.serialize(textboxes);
}



// Connects to WebSerial device if not connected
async function connectWebSerial() {
	if (serialDevice.isOpen()) return;
	await serialDevice.connect();
	document.querySelector('#webserial-disconnect').disabled = false;
}

// Reads logs from web serial to console
document.querySelector("#webserial-read").onchange = async function () {
	// Connects serial device if not connected already
	await connectWebSerial();

	// Sets reading state for serial device to state of checkbox
	serialDevice.setReading(this.checked);
};

// Uploads configuration over web serial
document.querySelector('#webserial-upload').onclick = async function () {
	if (!document.querySelectorAll(".config-container-open").length) {
		return;
	}

	// Gets serialized data
	const data = serializeConfig();

	// Connects to device if not connected already
	await connectWebSerial();

	// Sends serialized data to serial device
	this.disabled = true;
	await serialDevice.write(data);
	this.disabled = false;
};

// Disconnects serial device
document.querySelector('#webserial-disconnect').onclick = function () {
	serialDevice.close();
	this.disabled = true;
	document.querySelector("#webserial-read").checked = false;
};



// Loads local file to config
document.querySelector('#config-load').onclick = function () {
	const loadHandler = document.createElement('input');
	loadHandler.setAttribute('type', 'file');
	const reader = new FileReader();
	loadHandler.onchange = function () {
		reader.readAsText(this.files[0]);
	};
	reader.onload = function (event) {
		for (const line of event.target.result.split('\n')) {
			const [ key, value ] = line.split('=');
			if (!key) return;
			document.querySelector(`input[name=${key}]`).value = value;
		}
	};
	loadHandler.click();
};

// Save config to local file
document.querySelector('#config-save').onclick = function (event) {
	if (!confirm("Do you want to download your current configuration?")) return;
	const saveHandler = document.createElement('a');
	saveHandler.setAttribute('download', 'config.txt');
	saveHandler.setAttribute('href', `data:text/plain,${encodeURIComponent(serializeConfig())}`);
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
