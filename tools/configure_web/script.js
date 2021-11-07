// Show hides configuration items on click
document.querySelectorAll('.config-container').forEach((node) => {
	node.onclick = function (event) {
		if (event.target === this.lastElementChild) return;
		this.classList.toggle("config-container-open");
	};
});

// Function to convert form to a configuration string
function serializeConfig(lf) {
	const textboxes = document.querySelectorAll(".config-container-open .config-textbox");
	let output = '';
	for (const textbox of textboxes) {
		output += `${textbox.name}=${textbox.value}${lf}`;
	}
	output += lf;
	return output;
}



// Logs a messsage to the on-screen TTY
const tty = document.querySelector('.webserial-console');
function print(msg) {
	const atBottom = (tty.parentNode.scrollTop >= tty.scrollHeight - tty.parentNode.clientHeight);
	tty.textContent += msg;
	if (atBottom) tty.parentNode.scrollTop = tty.scrollHeight;
}

// Clears log
document.querySelector('.webserial-console-clear').onclick = function () {
	tty.textContent = '';
};

// Logs error if WebSerial is not available
if (!navigator.serial) {
	log("WebSerial API not available\n");
	document.querySelector("#webserial-read").disabled = true;
	document.querySelector("#webserial-upload").disabled = true;
}



// Reads logs from web-serial to console
document.querySelector("#webserial-read").onclick = async function () {
	if (!serialDevice) serialDevice = await new SerialDevice();
	document.querySelector("#webserial-disconnect").disabled = false;
	for await (const msg of serialDevice) {
		log(msg);
	}
};

// Uploads configuration over web-serial
let serialDevice = null;
document.querySelector('#webserial-upload').onclick = async function () {
	if (!document.querySelectorAll(".config-container-open").length) {
		log("\nNo configuration to send\n");
		return;
	}
	// Connects to device and sends serialized data
	let serialLogging = false;
	if (!serialDevice) {
		serialDevice = await new SerialDevice();
		serialLogging = true;
	}
	document.querySelector("#webserial-disconnect").disabled = false;
	serialDevice.write(serializeConfig('\n'));
	log("\nConfiguration sent\n");

	// Reads response data up to configuration is done
	if (serialLogging) return;
	let trimStartDone = false;
	let buffer = '';
	for await (const chunk of serialDevice) {
		// Buffers incomplete lines until next chunk and creates string without incomplete
		const splitted = (buffer + chunk).split("\r\n");
		buffer = splitted.pop();
		const msg = splitted.join('\n');

		// Trims out everything before configuration starts
		if (!trimStartDone) {
			const index = msg.indexOf('[');
			if (index >= 0) {
				log(msg.slice(index) + '\n');
				trimStartDone = true;
			}
		}
		// Exits after configuration ends
		else if (msg.startsWith("Configuration\n")) {
			log("Configuration saved\n");
			return;
		}
		// Prints logs
		else {
			log(msg + '\n');
		}
	}
};

// Disconnects curret web-serial device
document.querySelector('#webserial-disconnect').onclick = function () {
	serialDevice.close();
	serialDevice = null;
	this.disabled = true;
	log("\nDisconnected\n");
};
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
