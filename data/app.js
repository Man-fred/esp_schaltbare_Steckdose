var myAjax;
var myAjaxInterval;
var ajaxOnline = false;
var ajaxError = 0;
var Url = "/";
var aktuelleSeite = "";
var StatusArr;
var small = "";
var notifId = 0;
var userInit = 1;
(function (window, document) {
    var layout = document.getElementById('layout'),
            menu = document.getElementById('menu'),
            menuLink = document.getElementById('menuLink'),
            content = document.getElementById('main');

    if (window.location.hash)
        aktuelleSeite = window.location.hash.substring(1);

    function toggleClass(element, className) {
        var classes = element.className.split(/\s+/),
                length = classes.length,
                i = 0;

        for (; i < length; i++) {
            if (classes[i] === className) {
                classes.splice(i, 1);
                break;
            }
        }
        // The className is not found
        if (length === classes.length) {
            classes.push(className);
        }

        element.className = classes.join(' ');
    }

    function toggleAll(e) {
        var active = 'active';

        e.preventDefault();
        toggleClass(layout, active);
        toggleClass(menu, active);
        toggleClass(menuLink, active);
    }

    menuLink.onclick = function (e) {
        toggleAll(e);
    };

    content.onclick = function (e) {
        if (menu.className.indexOf('active') !== -1) {
            toggleAll(e);
        }
    };
    menu.onclick = function (e) {
        if (menu.className.indexOf('active') !== -1) {
            toggleAll(e);
        }
    };

    myAjax = new XMLHttpRequest();
    if (!myAjax)
    {
        notification("error", "Kann keine XMLHTTP-Instanz erzeugen");
        return false;
    } else {
        myAjax.onreadystatechange = LesenAjax;
    }

    load_page(aktuelleSeite, true);
}(this, this.document));

function load_page(seite, init=false) {
    Lesen();
    if (seite === "") {
        seite = "page0";
    }
    if (init || aktuelleSeite !== seite) {
        // Menü aktivieren
        if (!userInit && StatusArr[19] !== "Administrator" && (seite === "page3" || seite === "page5") ) {
            seite = "page0";
            notification("error", "keine Berechtigung");
        }
        var divs = document.querySelectorAll("#mainmenu .pure-menu-selected"), i;
        for (i = 0; i < divs.length; ++i) {
            divs[i].classList.remove('pure-menu-selected');
        }
        document.querySelector("#m_" + seite).classList.add('pure-menu-selected');

        // Seite aktivieren
        divs = document.querySelectorAll("#content .show");
        for (i = 0; i < divs.length; ++i) {
                divs[i].classList.add('hide');
                divs[i].classList.remove('show');
        }
        document.querySelector("#"+seite).classList.add('show');
        document.querySelector("#"+seite).classList.remove('hide');
        
        if (!init) {
            switch (aktuelleSeite) {
                case "page0" :
                    leavePage0();
                    break;
            }
            aktuelleSeite = seite;
        }
        switch (aktuelleSeite) {
            case "page0" :
                startPage0();
                small = "";
                break;
            case "page1" :
                loadTimer();
                small = "0";
                break;
            case "page3" :
                startPage3();
                small = "0";
                break;
            case "page4" :
                startPage4();
                small = "0";
                break;
            case "page5" :
                filesLoad();
                small = "0";
                break;
        }
        /*if (small === "") {
            document.querySelector("#buttonsSmall").classList.add('hide');
        } else {
            document.querySelector("#buttonsSmall").classList.remove('hide');
        }*/
    }
}

function startPage0()
{
    if (myAjax)
    {
        myAjaxInterval = setInterval(Lesen, 3000);
    }
}
function leavePage0()
{
    if (myAjaxInterval)
    {
        clearInterval(myAjaxInterval);
    }
}

function startPage3()
{
    var xmlhttp = new XMLHttpRequest();
    xmlhttp.onreadystatechange = function () {
        if (this.readyState === 4) {
            ajaxOnline = (this.status === 200);
            if (ajaxOnline) {
                var j = JSON.parse(this.responseText);
                document.getElementById("name_timer").value = j.name_timer;
                document.getElementById("name_r1").value = j.name_r1;
                document.getElementById("name_r2").value = j.name_r2;
                document.getElementById("name_r3").value = j.name_r3;
                document.getElementById("name_r4").value = j.name_r4;
                document.getElementById("ssid").value = j.ssid1;
                document.getElementById("pass").value = j.pass1;
                document.getElementById("timeserver").value = j.timeserver;
                document.getElementById("UpdateServer").value = j.update;
                document.getElementById("AdminName").value = j.name2;
                document.getElementById("AdminPasswort").value = j.pass2;
                document.getElementById("UserName").value = j.name3;
                document.getElementById("UserPasswort").value = j.pass3;
                document.getElementById("setup4").value = j.setup4;
                document.getElementById("setup5").value = j.setup5;
                document.getElementById("setup6").value = j.setup6;
                document.getElementById("setup7").value = j.setup7;
                document.getElementById("setup8").value = j.setup8;
                document.getElementById("setup9").value = j.setup9;
                document.getElementById("setup10").value = j.setup10;
                document.getElementById("setup11").value = j.setup11;
                document.getElementById("setup12").value = j.setup12;
                document.getElementById("setup13").value = j.setup13;
                document.getElementById("setup14").value = j.setup14;
                document.getElementById("setup15").value = j.setup15;
                notification("success", "Config geladen");
            } else {
                notification("error", "Config: Status " + this.status + " " + this.statusText);
            }
        }
    };
    xmlhttp.open("GET", "/daten.js", true);
    xmlhttp.setRequestHeader("Content-type", "text/json");
    xmlhttp.send();
}

function startPage4()
{
    var xmlhttp = new XMLHttpRequest();
    xmlhttp.onreadystatechange = function () {
        if (this.readyState === 4) {
            if (this.status === 200) {
                document.getElementById("logtable").innerHTML = this.responseText;
                notification("success", "Log geladen");
            } else {
                notification("error", "Log.txt: Status " + this.status + " " + this.statusText);
            }
        }
    };
    xmlhttp.open("GET", "/log.txt", true);
    xmlhttp.setRequestHeader("Content-type", "text/plain");
    xmlhttp.send();
}

function LesenAjax()
{
    if (this.readyState === 4) {
        ajaxOnline = (this.status === 200);
        var divs, i;
        if (ajaxOnline) {
            StatusArr = myAjax.responseText.split(";");
            divs = document.querySelectorAll(".btn,.btn1,.btn2");
            for (i = 0; i < divs.length; ++i) {
                if (parseInt(StatusArr[i]) == 1) {
                    divs[i].innerHTML = "ON";
                    divs[i].className = 'btn';
                } else {
                    divs[i].innerHTML = "OFF";
                    divs[i].className = 'btn1';
                }
            }
            divs = document.querySelectorAll(".btn00,.btn10,.btn20");
            for (i = 0; i < divs.length; ++i) {
                if (parseInt(StatusArr[i]) ) {
                    divs[i].className = 'btn00';
                } else {
                    divs[i].className = 'btn10';
                }
            }
            divs = document.querySelectorAll(".btn01,.btn11,.btn21");
            for (i = 0; i < divs.length; ++i) {
                if( parseInt(StatusArr[i+12]) ? parseInt(StatusArr[i]) : !parseInt(StatusArr[i]) ) {
                    divs[i].className = 'btn11';
                } else {
                    divs[i].className = 'btn21';
                }
            }
            document.getElementById("aktuell").innerHTML = StatusArr[16];
            document.getElementById("version").innerHTML = StatusArr[17];
            document.getElementById("ChipID").innerHTML = StatusArr[18];
            document.getElementById("userstatus").innerHTML = StatusArr[19];
            document.getElementById("NTPok").innerHTML = StatusArr[20];
            document.getElementById("RTCok").innerHTML = StatusArr[21];
            document.getElementById("IOok").innerHTML = StatusArr[22];
            document.getElementById("DISPLAYok").innerHTML = StatusArr[23];
            document.getElementById("NameTimer").innerHTML = StatusArr[24];
            document.getElementById("NameR1").innerHTML = StatusArr[25];
            document.getElementById("NameR2").innerHTML = StatusArr[26];
            document.getElementById("NameR3").innerHTML = StatusArr[27];
            document.getElementById("NameR4").innerHTML = StatusArr[28];
            document.title = StatusArr[24];
            if (userInit && StatusArr[19] === 'Administrator') {
                userInit = 0;
                document.querySelector("#m_page3").classList.remove('pure-menu-disabled');
                document.querySelector("#m_page5").classList.remove('pure-menu-disabled');
            }
        } else {
            notification("error", "Schalteraktualisierung: Status " + this.status + " " + this.statusText);
            divs = document.querySelectorAll(".btn,.btn1,.btn2");
            for (i = 0; i < divs.length; ++i) {
                divs[i].className = 'btn2';
            }
            divs = document.querySelectorAll(".btn00,.btn10,.btn20");
            for (i = 0; i < divs.length; ++i) {
                divs[i].className = 'btn20';
            }
        }
    }
}

function Schreiben(num)
{
    if (document.getElementsByTagName("input")[num - 1].checked)
        UrlGet = Url + "schalte.php?Relay=" + num + "&On=0";
    else
        UrlGet = Url + "schalte.php?Relay=" + num + "&On=1";
    myAjax.open("GET", UrlGet, true);
    myAjax.send();
}

function Taster(k) {
    var Butt = document.getElementById("button"+k);
    if ((Butt.innerHTML) === "ON") {
        var schalte = '0';
    } else {
        var schalte = '1';
    }
    UrlGet = Url + "schalte.php?Relay=" + (k + 1) + "&On=" + schalte;
    myAjax.open("GET", UrlGet, true);
    myAjax.send();
}

function Lesen()
{
    UrlGet = Url + "zustand.php";
    myAjax.open("GET", UrlGet, true);
    myAjax.send();
}


function DatensatzBilden(Id = "")
{
    var st = document.getElementById('dat' + Id).value;
    if (st === "")
        st = "1.1.0";
    var stSplit = st.split(".");
    var szeit = document.getElementById('zeit' + Id).value;
    var szeitSplit = szeit.split(":");
    var jahr = parseInt(stSplit[2], 10);
    if (jahr < 1000)
        jahr += 2000;
    var dt = new Date(Date.UTC(jahr, parseInt(stSplit[1] - 1, 10), parseInt(stSplit[0], 10), parseInt(szeitSplit[0], 10), parseInt(szeitSplit[1], 10), parseInt(szeitSplit[2], 10)));
    var Link = "settimer.php";
    Link += "?art=" + document.getElementById('art' + Id).value;
    Link += "&zeit=";
    Link += dt.valueOf() / 1000;
    Link += "&relais=";
    Link += document.getElementById('relais' + Id).value;
    Link += "&ein=" + document.getElementById('on' + Id).value;
    Link += "&aktiv=" + document.getElementById('aktiv' + Id).checked;
    Link += "&nachholen=" + document.getElementById('nachholen' + Id).checked;
    if (Id <= 250)
        Link += "&id=" + Id;
    self.location.href = Link;
}
function load() {
    var obj, dbParam, xmlhttp, myObj, x, txt = "";
    obj = {"table": "customers", "limit": 20};
    dbParam = JSON.stringify(obj);
    xmlhttp = new XMLHttpRequest();
    xmlhttp.onreadystatechange = function () {
        if (this.readyState == 4 && this.status == 200) {
            myObj = JSON.parse(this.responseText);
            txt += "<table border='1'>";
            for (x in myObj) {
                txt += "<tr><td>" + myObj[x].name + "</td></tr>";
            }
            txt += "</table>";
            document.getElementById("demo").innerHTML = txt;
        }
    };
    xmlhttp.open("POST", "jsonTimer.php", true);
    xmlhttp.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
    xmlhttp.send("x=" + dbParam);

}
function indexArt(a) {
    switch (a) {
        case "2":
            return 0;
        case "1":
            return 1;
        case "4":
            return 2;
        case "5":
            return 3;
        case "6":
            return 4;
        case "7":
            return 5;
        case "8":
            return 6;
        case "9":
            return 7;
        case "3":
            return 8;
        case "10":
            return 9;
        case "11":
            return 10;
        default:
            return -1;
    }
}
function myCreateFunction(a) {
    var table = document.getElementById("timertable");
    var row = table.insertRow(-1);
    if (a.id <= 250) {
        row.insertCell(0).innerHTML = a.id;
        row.insertCell(1).innerHTML = a.date_n;
    } else {
        row.insertCell(0).innerHTML = "";
        row.insertCell(1).innerHTML = "";
    }
    row.insertCell(2).innerHTML = "<input type=\"checkbox\" name=\"Aktiv\" id=\"aktiv" + a.id + "\" value=\"1\" " + (a.aktiv == "1" ? "checked=\"checked\"" : "") + ">";
    row.insertCell(3).innerHTML = "<input type=\"checkbox\" name=\"Nachholen\" id=\"nachholen" + a.id + "\" value=\"1\" " + (a.nachholen == "1" ? "checked=\"checked\"" : "") + ">";
    row.insertCell(4).innerHTML =
            "<select name=\"Art\" id=\"art" + a.id + "\" size=\"1\" style=\"text-align:center;\">" +
            "<option value=\"2\">Täglich</option><option value=\"1\">Einmalig</option><option value=\"4\">Montags</option><option value=\"5\">Dienstags</option>" +
            "<option value=\"6\">Mittwochs</option><option value=\"7\">Donnerstags</option><option value=\"8\">Freitags</option><option value=\"9\">Samstags</option>" +
            "<option value=\"3\">Sonntags</option><option value=\"10\">Wochenende/Feiertag</option><option value=\"11\">Werktage</option><option value=\"12\">Feiertage</option></select>";
    document.getElementById("art" + a.id).selectedIndex = indexArt(a.art);
    row.insertCell(5).innerHTML = "<input  name=\"Datum\" id=\"dat" + a.id + "\" value=\"" + a.date + "\" type=\"text\" size=\"10\" maxlength=\"10\" style=\"text-align:center;\">";
    row.insertCell(6).innerHTML = "<input  name=\"Zeit\" id=\"zeit" + a.id + "\" value=\"" + a.time + "\" type=\"text\" size=\"8\" maxlength=\"8\" style=\"text-align:center;\">";
    row.insertCell(7).innerHTML = "<select name=\"Relais\" id=\"relais" + a.id + "\" size=\"1\">" +
            "<option value=\"1\">Relais Nr.: 1</option>" +
            "<option value=\"2\">Relais Nr.: 2</option>" +
            "<option value=\"3\">Relais Nr.: 3</option>" +
            "<option value=\"4\">Relais Nr.: 4</option></select>";
    document.getElementById("relais" + a.id).selectedIndex = a.relais - 1;
    row.insertCell(8).innerHTML = "<select name=\"On\" id=\"on" + a.id + "\" size=\"1\"><option value=\"0\">OFF</option> <option value=\"1\">ON</option></select>";
    document.getElementById("on" + a.id).selectedIndex = a.ein;
    if (a.id == "251" || a.id == "252") {
        row.insertCell(9).innerHTML = "<a href=\"#\" onclick=\"DatensatzBilden(" + a.id + ");return false;\">Neu</a>";
        row.insertCell(10).innerHTML = "";
    } else {
        row.insertCell(9).innerHTML = "<a href=\"#\" onclick=\"DatensatzBilden(" + a.id + ");return false;\">Anpassen</a>";
        row.insertCell(10).innerHTML = "<a href=\"delete.php?Nr=" + a.id + "\" >Löschen</a>";
    }
}

function myDeleteFunction() {
    document.getElementById("timertable").deleteRow(0);
}

function loadTimer() {
    var table = document.getElementById("timertable");
    table.innerHTML = "";
    var row = table.createTHead().insertRow(-1);
    row.innerHTML = "<th>Index</th><th>Nächstes Ereignis</th><th>Aktiv</th><th>Nachholen</th><th>Art</th><th>Datum</th><th>Zeit</th><th>Relais</th><th>On/Off</th><th>Aktion</th><th>Löschen</th>";
    myCreateFunction({id: "251", aktiv: "1", nachholen: "1", art: "-1", relais: -1, on: -1, time: "", date: "", date_n: ""});

    var xmlhttp = new XMLHttpRequest();

    xmlhttp.onreadystatechange = function () {
        if (this.readyState == 4 && this.status == 200) {
            var j = JSON.parse(this.responseText);
            for (var x in j) {
                if (j[x].id == "Z") {
                    document.getElementById("aktuell").innerHTML = j[x].date + "&nbsp;" + j[x].time;
                } else
                    myCreateFunction(j[x]);
            }
            myCreateFunction({id: "252", aktiv: "1", nachholen: "1", art: "-1", relais: -1, on: -1, time: "", date: "", date_n: ""});
        }
    };
    xmlhttp.open("POST", "timer.json", true);
    xmlhttp.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
    xmlhttp.send();
}

function filesLoad() {
    var table = document.getElementById("filetable");
    table.innerHTML = "";
    var row = table.createTHead().insertRow(-1);
    row.innerHTML = "<th>Dateiname</th><th>Typ</th><th>Größe</th><th>Download</th><th>Löschen</th>";

    var xmlhttp = new XMLHttpRequest();

    xmlhttp.onreadystatechange = function () {
        if (this.readyState == 4 && this.status == 200) {
            var table = document.getElementById("filetable");
            var tbody = table.createTBody();
            var j = JSON.parse(this.responseText);
            for (var x in j) {
                if (j[x].type === "file") {
                    var row = tbody.insertRow(-1);
                    row.insertCell(0).innerHTML = j[x].name;
                    row.insertCell(1).innerHTML = j[x].type;
                    row.insertCell(2).innerHTML = j[x].size;
                    row.insertCell(3).innerHTML = "<a href=\"/" + j[x].name + "\" download>Download</a>";
                    row.insertCell(4).innerHTML = "<a href=\"#page5\" onclick=\"filesDelete('" + j[x].name + "');return false;\">Löschen</a>";
                } else {

                }
            }
        }
    };
    xmlhttp.open("GET", "/list?dir=/", true);
    xmlhttp.setRequestHeader("Content-type", "text/json");
    xmlhttp.send();
}
function filesDelete(a)
{
    var xmlhttp = new XMLHttpRequest();
    xmlhttp.onreadystatechange = function () {
        if (this.readyState == 4) {
            if (this.status == 200) {
                //var j = JSON.parse(this.responseText);
                notification("success", "Datei gelöscht");
            } else {
                notification("error", "Datei nicht gelöscht: Status " + this.status + " " + this.statusText);
            }
        }
    };
    xmlhttp.open("GET", "/delete?file=/" + a, true);
    xmlhttp.setRequestHeader("Content-type", "text/json");
    xmlhttp.send();
}
function filesUpload()
{
    var upload =  document.getElementById("uploadfile");
    var formData = new FormData();
    formData.append("upload", upload.files[0]);
    var xmlhttp = new XMLHttpRequest();
    xmlhttp.onreadystatechange = function () {
        if (this.readyState == 4) {
            if (this.status == 200) {
                //var j = JSON.parse(this.responseText);
                notification("success", "Datei geladen");
                document.getElementById("uploadfile").innerHTML = '';
            } else {
                notification("error", "Datei nicht geladen: Status " + this.status + " " + this.statusText);
            }
        }
    };
    xmlhttp.open("POST", "/upload.json", true);
    //xmlhttp.setRequestHeader("Content-type", "multipart/form-data");
    //xmlhttp.setRequestHeader("Content-type", "false");
    xmlhttp.send(formData);
}

function notification(state, message) {
    var notif = document.getElementById("notification");
    notifId++;
    var n1 = document.createElement('notif_' + notifId);
    notif.appendChild(n1);
    n1.innerHTML = message;
    n1.classList.add('pure-button');
    n1.classList.add('button-' + state);
    n1.classList.add('w3-animate-fading');
    var n1Timeout = setTimeout(function () {
        n1.parentNode.removeChild(n1);
        //n1.classList.remove('w3-animate-fading');
    }, 10000);
    n1.addEventListener("click", function () {
        //n1.classList.remove('w3-animate-fading');
        clearTimeout(n1Timeout);
        n1.parentNode.removeChild(n1);
    });
    //notif.classList.add('w3-animate-fading');
}
