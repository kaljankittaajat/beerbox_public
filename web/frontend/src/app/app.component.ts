/********************************
 *Beerbox app by kaljankittaajat*
 ********************************/
import {Component, OnInit} from '@angular/core';
import { HttpClient } from '@angular/common/http';
import * as moment from 'moment';

interface Data {
  temp: number;
  time: any;
  weight: number;
}

interface Device {
  id: number;
  name: string;
  weight_min: number;
  weight_max: number;
  temp_min: number;
  temp_max: number;
  name_temp: string;
  weight_min_temp: number;
}

interface Colors {
  domain: string [];
}
@Component({
  selector: 'app-root',
  templateUrl: './app.component.html',
  styleUrls: ['./app.component.css'],
})

export class AppComponent implements OnInit {
  title = 'beerbox';
  data: Data = {
    temp: 0,
    time: moment.now(),
    weight: 0
  };
  device: Device = {
    id: 1,
    name: '',
    weight_min: 0,
    weight_max: 0,
    temp_min: 0,
    temp_max: 0,
    name_temp: '',
    weight_min_temp: 0
  };
  history: any = [];
  monthly: any = [];
  total_consumption: any = 0;
  month = moment(moment.now()).month() + 1; // returns 0-11 so we add 1 to have 1-12
  year = moment(moment.now()).year();
  month_text = '';
  nextDisabled: boolean = true;
  weight_chart: any = [];

  constructor(private http: HttpClient) {
  }

  single: any = [];
  view: any = [700, 400];
  cloud = ''; // use empty if backend on same server, server address if on different server or http://localhost:8080 if you are testing backend locally

  // options for ngxcharts
  showXAxis = true;
  showYAxis = true;
  showGridLines = false;
  gradient = false;
  showLegend = false;
  showDataLabel = false;
  showXAxisLabel = true;
  xAxisLabel = 'Day';
  xAxisLabel2 = 'Time';
  showYAxisLabel = true;
  yAxisLabel = 'Consumption (kg)';
  yMin = 0;
  yMax = 10;
  yAxisLabelLine = 'Weight';
  xScaleMin = moment(moment.now()).subtract(7, "days").toDate().getTime();
  xScaleMax = moment(new Date()).toDate().getTime();
  colors: Colors = {
    domain: []
  };
  roundEdges = false;

  dayDisabled: boolean = false;
  weekDisabled: boolean = true;
  twoWeeksDisabled: boolean = false;
  chart_text: string = '7 days'
  weekday_color = '#50ABCC';
  weekend_color = '#AD6886';

  getData() {
    const url = this.cloud + '/latest/' + this.device.id;
    this.http.get(url).subscribe((res) => {
      let arr: any;
      arr = res;
      this.calculateWeight(arr);
      for (let i in arr) {
        this.data.temp = arr[i].temp;
        this.data.time = arr[i].time;
        this.data.weight = arr[i].weight;
      }
    });
  }

  getDevice() {
    const url = this.cloud + '/device/' + this.device.id;
    this.http.get(url).subscribe((res) => {
      let arr: any;
      arr = res;
      for (let ay in arr) {
        this.device.name = arr[ay].name
        this.device.weight_min = arr[ay].weight_min;
        this.device.weight_max = arr[ay].weight_max;
        this.device.temp_min = arr[ay].temp_min;
        this.device.temp_max = arr[ay].temp_max;
        this.device.name_temp = this.device.name;
        this.device.weight_min_temp = this.device.weight_min;
      }
    });
  }

  onSubmitEditDevice() {
    const url = this.cloud + '/device/' + this.device.id;
    const body = {
      name: this.device.name_temp,
      weight_min: this.device.weight_min_temp
    };
    this.http.put <any>(url, body)
      .subscribe({
          next: data =>  {
            console.log(data); //debug
            this.getDevice();
          },
          error: error => {
            error.message;
            console.log(error.message); //debug
          }
        }
      );
  }



  // this function also converts database datetime to date objects with correct timezone.
  calculateWeight(array: any []) {
    let weight: number;
    let dt: moment.Moment;
    for (let i in array) {
      if (array[i].length !== 0) {
        weight = (array[i].sensor_1 + array[i].sensor_2 + array[i].sensor_3 + array[i].sensor_4) / 1000; // add sensor data and divide by 1000 to turn grams to kilograms
        array[i].weight = weight;
        dt = moment(array[i].time);
        array[i].time = dt.toDate();
      }
    }
  }

  getHistory() {
    const url = this.cloud + '/history/' + this.device.id;
    this.http.get(url).subscribe((res) => {
      this.history = res;
      this.calculateWeight(this.history);
      this.getMonthlyChart();
    });
  }

  getWeightChart() {
    const url = this.cloud + '/weight_chart/' + this.device.id;
    this.http.get(url).subscribe((res) => {
      this.weight_chart = res;
      this.calculateWeight(this.weight_chart)
      let a: any = [];
      a.push({
        name: 'weight',
        series: Array<{ name: Date, value: number }>()
      });
      for (let i in res) {
        a[0].series.push({
          name: this.weight_chart[i].time,
          value: this.weight_chart[i].weight
        });
      }
      this.weight_chart = a;
    });
  }

  // cumulative weight loss function, returns [total, weight_on_last_index], second one is needed for day to day calculation so we don't lose weight loss that happens between days
  cumwl(array: any [], last = 0) {
    let x = last;
    let total = 0;
    for (let val in array) {
      if (x > array[val].weight) {
        total += x - array[val].weight;
      }
      x = array[val].weight;
    }
    return [total, x];
  }

  getMonthlyChart() {
    this.month_text = moment(this.month, 'MM').format('MMMM');
    this.nextDisabled = (this.month >= moment(moment.now()).month() + 1 && this.year >= moment(moment.now()).year());
    this.colors.domain = []; // empty colorscheme
    const max = moment(new Date(this.year, this.month - 1)).endOf('month').toDate().getDate();
    let array: any [] = this.history; // filter doesn't work on this.history straight so we copy it to new array with slightly different definition
    let filteredArray: any = []; // array that gets filtered datapoints
    let chartData: any = []; // array we use to plot monthly chart
    let last: number = 0;
    let total: number = 0
    let sum: number [] = [0,0];
    let colorArray: string [] = [];
    let wd: any;
    for (let i = 1; i <= max; i++) {
      //filter daily data for selected year, month and day from array that contains all data
      filteredArray.push(array.filter(time => moment(time.time).toDate().getDate() === i && moment(time.time).toDate().getMonth() + 1 === this.month && moment(time.time).year() === this.year));
      // get weekday of the day to make weekend bars different color than weekday bars;
      wd = moment(new Date(this.year, this.month - 1, i)).toDate().getDay();
      // create colorscheme for bar chart based on first 7 days, after that it repeats from start.
      if (i <= 7) {
        colorArray.push((wd === 6 || wd === 0) ? this.weekend_color : this.weekday_color)
      }
      // push data to chartData array
      if (filteredArray[i - 1].length !== 0) {
        sum = this.cumwl(filteredArray[i - 1], last);  // returns [sum, last weight for day]
        chartData.push({
          name: i.toString(),
          value: sum[0]
        });
        last = sum[1];
        total += sum[0]
      } else {
        //push value 0 to day that doesn't have any data points gathered
        chartData.push({
          name: i.toString(),
          value: 0
        });
      }
    }
    //set monthly total consumption, color scheme for bars and data for bar chart
    this.total_consumption = total.toFixed(1);
    this.colors.domain = colorArray;
    this.monthly = chartData;
  }

  nextMonth() {
    this.month += 1;
    if (this.month === 13) {
      this.year += 1;
      this.month = 1;
    }
    this.getMonthlyChart();
  }

  currentMonth() {
    this.month = moment(moment.now()).month() + 1;
    this.year = moment(moment.now()).year();
    this.getMonthlyChart();
  }

  previousMonth() {
    this.month -= 1;
    if (this.month === 0) {
      this.year -= 1;
      this.month = 12;
    }
    this.getMonthlyChart();
  }

  lastDay() {
    this.xScaleMin = moment(moment.now()).subtract(1, "days").toDate().getTime();
    this.dayDisabled = true;
    this.weekDisabled = false;
    this.twoWeeksDisabled = false;
    this.chart_text = 'day'
    this.getWeightChart();
  }

  lastWeek() {
    this.xScaleMin = moment(moment.now()).subtract(7, "days").toDate().getTime();
    this.dayDisabled = false;
    this.weekDisabled = true;
    this.twoWeeksDisabled = false;
    this.chart_text = 'week'
    this.getWeightChart();
  }

  lastTwoWeeks() {
    this.xScaleMin = moment(moment.now()).subtract(14, "days").toDate().getTime();
    this.dayDisabled = false;
    this.weekDisabled = false;
    this.twoWeeksDisabled = true;
    this.chart_text = '2 weeks'
    this.getWeightChart();
  }

  // fetch all data on init, fetch latest data for navbar once every 10 seconds and chart data once every minute
  ngOnInit() {
    this.getData();
    this.getDevice();
    this.getHistory();
    this.getWeightChart();
    setInterval(() => {
      this.getData();
    }, 10 * 1000);
    setInterval(() => {
      this.getHistory();
      this.getWeightChart();
    }, 60 * 1000);
  }
}
