
function dgEBI(id){
    return document.getElementById(id);
}
    
function sk(k, i){
    h="KEY";
    if (i==5) h="UID"
    dgEBI('k' + i).innerHTML=h + " " + i + ":";
    e=0;
    l=0;
    for (var x = 0; x < k.length; x+=2) {
        var c = k.charAt(x) + k.charAt(x + 1);
        var r=/^([0-9a-f]{2}){1,2}$/i;
        if (r.test(c)){
            dgEBI('k' + i).insertAdjacentHTML('beforeend', " " + c);
            l+=1;
        }
        else{
            e+=1;
        }
    }
    if (l!=16) e+=1;
    return(e);
}

function sd(d){
    h="URL    : ";
    dgEBI('u').innerHTML=h + " " + d;
}

function updateform(url, data, rhandle){
    xhruf = new XMLHttpRequest();
    xhruf.open("POST", url, true);
    xhruf.setRequestHeader('Content-Type', 'application/json')
    xhruf.onreadystatechange = function () {
        if (xhruf.readyState == 4 && xhruf.status == 200) {
            try {
                JSON.parse(xhruf.responseText);
            }
            catch(ex) {
                return false;
            }
            let o = JSON.parse(xhruf.responseText);
            if (rhandle){
				rhandle(o);
			}
			else{
				window.location.href = window.location.origin + window.location.pathname;
			}
        }
    };
    xhruf.send(JSON.stringify(data));
}

function rd(d){ 
    console.log(d);
    e=0;
    let o = {};
    try {
        o = JSON.parse(d);
        JSON.parse(d);
        e+=sk(o.k0,0);
        e+=sk(o.k1,1);
        e+=sk(o.k2,2);
        e+=sk(o.k3,3);
        e+=sk(o.k4,4);
        sd(o.lnurlw_base);
    }
    catch(ex) {
        e=1;
    }
    i=dgEBI('next');
    if (e > 0) {
        c='#ffffff';
        cn='#adadad';
        i.form.setAttribute("data-x","1")
    }
    else {
        c='#ddffdd';
        cn='#3EAF7C';
        i.form.removeAttribute("data-x")
    }
    dgEBI('next').style.backgroundColor=cn;
    dgEBI('ki').style.backgroundColor=c;
    if (e==0){
        i=dgEBI('next');
    }
    console.log(e);
    return(e);
}

let t = Date.now();
let to = 0;
let su = 0;

function scanuid(){
    xhrsc = new XMLHttpRequest();
    xhrsc.open("POST", "/uid", true);
    xhrsc.setRequestHeader('Content-Type', 'application/json');
    xhrsc.onload = function() {
      if (xhrsc.readyState == 4 && xhrsc.status == 200) {
        data = JSON.parse(xhrsc.responseText);
        if (data['uid'] != "") {
            dgEBI('uid').value=data['uid'];
        }
      }
    }
    xhrsc.send("{'msg':'**Present card**'}");
}

function pro(ti, msg, cb){
    dgEBI('proti').innerHTML=ti;
    dgEBI('promsg').innerHTML=msg;
    dgEBI('prook').addEventListener('click', function(){
        cb();
        dgEBI('sm6').classList.remove("open");
    });
    dgEBI('sm6').classList.add("open");
}

function isValidHttpUrl(string) {
  let url;
  try {
    url = new URL(string);
  } catch (_) {
    return false;
  }
  return url.protocol === "http:" || url.protocol === "https:";
}


window.onload = function() {
    if ((dgEBI('lnurlw_base').value.length < 6) && (dgEBI('k0').value.length!=32)){
        col = document.getElementsByClassName("onl");
        for (let i = 0; i < col.length; i++) {
          col[i].style.visibility = "visible";
          col[i].style.display = "table";
        }
    }   
    if ((dgEBI('wallet_url').value.length < 6)){
        col = document.getElementsByClassName("now");
        for (let i = 0; i < col.length; i++) {
          col[i].style.visibility = "hidden";
          col[i].style.display = "none";
        }
    }   
    if (wifista != "1"){
		dgEBI("mode-ap").checked = true;
        col = document.getElementsByClassName("sta");
        for (let i = 0; i < col.length; i++) {
          col[i].style.visibility = "hidden";
          col[i].style.display = "none";
        }
    }   
    if (wifista == "1"){
		dgEBI("mode-sta").checked = true;
	}
    let i = dgEBI('ki');
    i.addEventListener('keyup', function () {
        t = Date.now();
        console.log(t);
        if(to){
            clearTimeout(to);
        }
        to = setTimeout(() => {
                let s = Date.now();
                if (s - t > 50){
                    rd(this.value);
                }
            }
        , 100);
    });
    su = setInterval(() => {
            let s = Date.now();
            if (s - t > 50){
                scanuid();
            }
        }
    , 1000);
    dgEBI('ki').dispatchEvent(new Event("keyup"));
    //modal
    document.addEventListener('click', function (e) {
        console.log(e);
        e = e || window.event;
        var target = e.target || e.srcElement;
        if (target.hasAttribute('data-toggle') && target.getAttribute('data-toggle') == 'modal') {
            if (target.hasAttribute('data-target')) {
                var m_ID = target.getAttribute('data-target');
                dgEBI(m_ID).classList.add('open');
                e.preventDefault();
            }
        }
        if (target.tagName.toLowerCase() == 'label'){
            console.log(target.previousElementSibling);
            target.previousElementSibling.focus();
            e.preventDefault();
        }
       if (target.type == 'button'){
            fe = target.form.elements;
            console.log(fe);
            if (target.form.hasAttribute("data-x")){
                return;
            }
            let data = {}
            for (i=0; i<fe.length; i++){
                if (fe[i].type == "radio"){
                    if (fe[i].checked){
                        data[fe[i].name] = fe[i].value;
                    }
                }
                else{
                    data[fe[i].id] = fe[i].value;
                }
            }
            if (target.hasAttribute('data-loc') && (dgEBI('card_new').value != "") && (dgEBI('uid').value != "")){
                loc = target.getAttribute('data-loc');
                let dk = "00000000000000000000000000000000";
                if ((dgEBI('k0').value == "") || (dgEBI('k0').value == dk)){
                    dgEBI('k0').value = makeid(32);
                }
                if ((dgEBI('k1').value == "") || (dgEBI('k1').value == dk)){
                    let tkey = makeid(32);
                    dgEBI('k1').value = tkey;
                    dgEBI('k3').value = tkey
                }
                if ((dgEBI('k2').value == "") || (dgEBI('k2').value == dk)){
                    let tkey = makeid(32);
                    dgEBI('k2').value = tkey;
                    dgEBI('k4').value = tkey
                }
                let tx_limit = dgEBI('tx_limit').value;
                if (isNaN(tx_limit)){
                    dgEBI('tx_limit').value = 1000;
                }
                let daily_limit = dgEBI('daily_limit').value;
                if (isNaN(daily_limit)){
                    dgEBI('daily_limit').value = 10000;
                }
                let par = document.createElement('a');
                if (dgEBI('host_new').value !=""){
                    if (dgEBI('host_new').value != ""){
                        par.href = dgEBI('host_new').value;
                    }
                }
                if (isValidHttpUrl(dgEBI('host_new').value)){
                    par.href = dgEBI('host_new').value;
                }
                else{
                    par.href = "https://legend.lnbits.com";
                }
                dgEBI('host_new').value = par.href;
                dgEBI('wallet_host').value = dgEBI('host_new').value;
                wurl = par.origin;
                lbnewwal(dgEBI('card_new').value, dgEBI('uid').value, 
                        function(usr, wal, uid, card){
                            dgEBI('card_name').value=card['card_name'];
                            dgEBI('lnurlw_base').value="lnurlw://" + par.host + "/boltcards/api/v1/scan/" + card['external_id'];
                            dgEBI('kesave').click();
                        });
                return;
            }
            if (target.form.hasAttribute("data-j")){
                console.log("json ");
                jf = target.form.getAttribute("data-j");
                dj = dgEBI(jf).value;
                data = JSON.parse(dj);
            }
            //console.log(target.form.hasAttribute("data-t"));
            if (target.form.hasAttribute("data-t")){
                url = document.location.origin + target.form.getAttribute("data-t");
                console.log(url);
                console.log(data);
				rhandle = null;
                if (target.form.hasAttribute("data-r")){
					rhandle = window[target.form.getAttribute("data-r")];
				}
                updateform(url, data, rhandle);
            }
        }
        // Close modal window with 'data-dismiss' attribute or when the backdrop is clicked
        if ((target.hasAttribute('data-dismiss') && target.getAttribute('data-dismiss') == 'modal') || target.classList.contains('modal')) {
            var modal = document.querySelector('[class="modal open"]');
            modal.classList.remove('open');
            e.preventDefault();
        }
    }, false);   
}

function lbnewwal(nam, uid, cb){
    console.log(wurl);
    curl = wurl + "/wallet?nme=" + nam;
    if (dgEBI('wallet_url').length > 0){
        curl = dgEBI('wallet_url');
    }
    xhrcw = new XMLHttpRequest();
    xhrcw.open("GET", wurl + "/wallet?nme=" + nam, true);
    xhrcw.setRequestHeader('Content-Type', 'application/json');
    xhrcw.onerror = function(e) {
        console.log(e);
        pro("Could not create wallet", "Could not connect to host. Make shure your wallet host is correct." , function(){});
    }
    xhrcw.onload = function() {
        if (xhrcw.status == 200) {
            console.log(xhrcw.responseURL);
            qs = xhrcw.responseURL.split('?').pop();
            var urlParams = new URLSearchParams(qs);
            let usr_id = "";
            let wal_id = "";
            let res = xhrcw.responseText;
            let b = res.match("<strong>Admin key: <\/strong><em>(.*?)<\/em>");
            console.log(b);
            let ap = b[1];
            console.log(ap);
            if(urlParams.has('usr')) {
                usr=urlParams.get('usr');
            }
            if(urlParams.has('wal')) {
                wal=urlParams.get('wal');
            }
            dgEBI('wallet_url').value = dgEBI('wallet_host').value + "wallet?usr=" + usr;
            lbabx(nam, uid, usr, wal, ap, cb);
        }
        else{
            console.log("wallet creation failed, trying to setup account first.");
            lbnewacc(nam, uid, cb);
            //pro("Could not create wallet", xhrcw.responseText , function(){alert("ok")});
        }
    };
    console.log(xhrcw.send());
}
  


function lbnewacc(nam, uid, cb){
    //console.log("lbcbc called")
    accountn = {
      "name": nam
    };      
    xh = new XMLHttpRequest();
    xh.open("POST", wurl + "/api/v1/account", true);
    xh.setRequestHeader('Content-Type', 'application/json');
    xh.setRequestHeader('accept', 'application/json');
    xh.onerror = function(e) {
        pro("Could not create account", xhrcw.responseText , function(){alert("ok")});
    }
    xh.onload = function() {
        if ((xh.status == 200) || (xh.status == 201)) {
            try {
                account = JSON.parse(xh.responseText);
                dgEBI('wallet_url').value = dgEBI('wallet_host').value + "wallet?usr=" + account.user;
                lbabx(nam, uid, account.user, account.id, account.adminkey, cb);
            }
            catch(ex) {
                console.log(ex);
                return false;
            }
        }
        else{
            try {
                edetail = JSON.parse(xh.responseText);
                pro("Could not create account", edetail['detail'] , function(){}); 
            }
            catch(ex) {
                pro("Could not create account", "unknown error" , function(){}); 
                return false;
            }
        }
    }
    xh.send(JSON.stringify(accountn));
}

function lbabx(nam, uid, usr, wal, ap, cb){
    console.log("lbabx called")
    xla = new XMLHttpRequest();
    xla.open("GET", wurl + "/extensions?usr=" + usr + "&enable=boltcards", true);
    xla.setRequestHeader('Content-Type', 'application/json');
    xla.setRequestHeader('X-Api-Key', usr);
    xla.onerror = function(e) {
        console.log(e);
        pro("Could not enable cards extension", "Could not connect to host. Make shure your wallet host is correct." , function(){});
    }
    xla.onload = function() {
      if (xla.status == 200) {
        lbcbc(nam, uid, usr, wal, ap, cb);
      }
    }
    xla.send();
}

function lbcbc(nam, uid, usr, wal, ap, cb){
    //console.log("lbcbc called")
    card = {
      "card_name": nam,
      "uid": uid,
      "counter": 0,
      "tx_limit": dgEBI('tx_limit').value,
      "daily_limit": dgEBI('daily_limit').value,
      "enable": true,
      "k0": dgEBI('k0').value,
      "k1": dgEBI('k1').value,
      "k2": dgEBI('k2').value
    };      
    xh = new XMLHttpRequest();
    xh.open("POST", wurl + "/boltcards/api/v1/cards", true);
    xh.setRequestHeader('Content-Type', 'application/json');
    xh.setRequestHeader('accept', 'application/json');
    xh.setRequestHeader('X-Api-Key', ap);
    xh.onerror = function(e) {
        console.log(e);
        pro("Could not create card", "Could not connect to host. Make shure your wallet host is correct." , function(){});
    }
    xh.onload = function() {
        if ((xh.status == 200) || (xh.status == 201)) {
            try {
                cardn = JSON.parse(xh.responseText);
                cb(usr, wal, uid, cardn);
            }
            catch(ex) {
                console.log(ex);
                return false;
            }
        }
        else{
            try {
                edetail = JSON.parse(xh.responseText);
                pro("Could not create card", edetail['detail'] , function(){}); 
            }
            catch(ex) {
                pro("Could not create card", "unknown error" , function(){}); 
                return false;
            }
        }
    }
    xh.send(JSON.stringify(card));
}

function makeid(length) {
    let result = '';
    const characters = '0123456789abcdef';
    const charactersLength = characters.length;
    let counter = 0;
    while (counter < length) {
      result += characters.charAt(Math.floor(Math.random() * charactersLength));
      counter += 1;
    }
    return result;
}    

function rhndwifi(r){
	dgEBI("sm2mod").innerHTML = '<h3>Wifi Setup</h3><div style="color:#ff0000; font-size: 1.5em;">The wifi-settings have been updated. You might need to change your wifi and check bolty for the ip-adress to connect to.<br/><br/></div>';
}
