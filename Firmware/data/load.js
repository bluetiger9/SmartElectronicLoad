class UI {
  constructor(setHandler) {
    this.setCurrentElem = document.getElementById("set-current");
    this.loadCurrentElem = document.getElementById("load-current");
    this.loadVoltageElem = document.getElementById("load-voltage");
    this.loadPowerElem = document.getElementById("load-power");
    this.temperatureElem = document.getElementById("temperature");
    this.fanSpeedElem = document.getElementById("fan-speed");

    this.overlayElem = document.getElementById("overlay");
    this.containerElem = document.getElementById("container");

    this.setCurrentForm = document.getElementById("set-current-form");
    this.setCurrentInput = document.getElementById("set-current-input");

    this.setFanSpeedCurrentForm = document.getElementById("fan-speed-form");
    this.setFanSpeedInput = document.getElementById("fan-speed-input");

    let that = this;

    this.setCurrentForm.onsubmit = function(event) {
      event.preventDefault();

      that.hideOverlay();

      setHandler("current", that.setCurrentInput.value);
    }

    this.setCurrentElem.onclick = function() {
      that.showOverlay("current-set-form-container");
    }

    this.setFanSpeedCurrentForm.onsubmit = function(event) {
      event.preventDefault();

      that.hideOverlay();

      setHandler("fan-speed", that.setFanSpeedInput.value);
    }

    this.fanSpeedElem.onclick = function() {
      that.showOverlay("fan-speed-form-container");
    }

    this.hideOverlay();
  }

  setVoltage(voltage) {
    this.loadVoltageElem.textContent = this._fourDigits(voltage);
  }

  setCurrent(current) {
    this.loadCurrentElem.textContent = this._fourDigits(current);
  }

  setPower(power) {
    this.loadPowerElem.textContent = this._fourDigits(power);
  }

  setTemperature(temperature) {
    this.temperatureElem.textContent = temperature.toFixed(1);
  }

  setSetCurrent(current) {
    this.setCurrentElem.textContent = this._fourDigits(current);
  }

  setFanSpeed(fanSpeed) {
    this.fanSpeedElem.textContent = fanSpeed.toFixed(0);
  }

  showOverlay(childId) {
    this.overlayElem.style.display = "flex";
    this.overlayElem.style.top = `20px`
    this.overlayElem.style.left = `20px`;

    for (var idx in this.overlayElem.childNodes){
      let child = this.overlayElem.childNodes[idx];
      if (child.id == childId) {
        child.style.display = "flex";
      }
    };
  }

  hideOverlay() {
    this.overlayElem.style.display = "none";
    for (var idx in this.overlayElem.childNodes){
      let child = this.overlayElem.childNodes[idx];
      if (child.nodeType === Node.ELEMENT_NODE) {
        this.overlayElem.childNodes[idx].style.display = "none";
      }
    };
  }

  _fourDigits(number) {
    if (number < 10.0) {
      return number.toFixed(3);
    } else if (number < 100.0) {
      return number.toFixed(2);
    } else if (number < 1000.0) {
      return number.toFixed(1)
    } else {
      return number;
    }
  }
}

class API {
  constructor(baseUrl) {
    this.baseUrl = baseUrl;
  }

  async getVoltage() {
    const voltageResponse = await this._get("/voltage");
    return voltageResponse['voltage'];
  }

  async getCurrent() {
    const currentResponse = await this._get("/current");
    return currentResponse['current'];
  }

  async getTemperature() {
    const currentResponse = await this._get("/temperature");
    return currentResponse['temperature'];
  }

  async setCurrent(current) {
    await this._put("/current", `${current.toFixed(3)}`);
  }

  async setFanSpeed(fanSpeedPercent) {
    const fanSpeed = fanSpeedPercent / 100.0;
    await this._put("/fan", `${fanSpeed.toFixed(3)}`);
  }

  async _get(path) {
    const response = await fetch(`${this.baseUrl}${path}`);
    return await response.json();
  }

  async _put(path, body) {
    const response = await fetch(`${this.baseUrl}${path}`, {
      method: "PUT",
      body: body,
      headers: {
        "Content-Type": "application/json"
      }
    });
    //return await response.json();
    return await response.text();
  }
}

class App {
  updateInterval = 1000;

  constructor(baseUrl) {
    if (baseUrl == undefined) {
      baseUrl = window.location.origin + "/api";
    }

    console.info(`Initializing Load with base URL: ${baseUrl}`)

    let that = this;

    this.ui = new UI((what, value) => that.setHandler(what, value));
    this.api = new API(baseUrl);

    this.scheduleUpdate();
  }

  scheduleUpdate() {
    let that = this;
    setTimeout(() => that.update(that), this.updateInterval);
  }

  async update() {
    // voltage
    const voltage = await this.api.getVoltage();
    this.ui.setVoltage(voltage);

    // current
    const current = await this.api.getCurrent();
    this.ui.setCurrent(current);

    // power
    const power = voltage * current;
    this.ui.setPower(power);

    // temperature
    const temperature = await this.api.getTemperature();
    this.ui.setTemperature(temperature);

    this.scheduleUpdate();
  }

  async setCurrent(current) {
    await this.api.setCurrent(current);

    this.ui.setSetCurrent(current);
  }

  async setFanSpeed(fanSpeed) {
    await this.api.setFanSpeed(fanSpeed);

    this.ui.setFanSpeed(fanSpeed);
  }

  setHandler(what, value) {
    console.info(`Set ${what} ${value}`);
    if (what == "current") {
      let current = Number(value);
      this.setCurrent(current);

    } else if (what == "fan-speed") {
      let fanSpeed = Number(value);
      this.setFanSpeed(fanSpeed);
    }
  }
}



const urlParams = new URLSearchParams(window.location.search);
const hostParam = urlParams.get('host');
const baseUrl = hostParam ? `http://${hostParam}/api` : undefined;

app = new App(baseUrl);