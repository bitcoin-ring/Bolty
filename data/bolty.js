function sd(d){
    h="URL    : ";
    document.getElementById('u').innerHTML=h + " " + d;
}

let t = Date.now();
let to = 0;

function updatestatus(){
    xmlhttp = new XMLHttpRequest();
    xmlhttp.open("POST", "/status", true);
    xmlhttp.setRequestHeader('Content-Type', 'application/json')
    xmlhttp.onreadystatechange = function () {
        if (xmlhttp.readyState == 4 && xmlhttp.status == 200) {
            try {
                JSON.parse(xmlhttp.responseText);
            }
            catch(ex) {
                return false;
            }
            let o = JSON.parse(xmlhttp.responseText);
            document.getElementById('st').innerHTML = o.status;
            if (o.app != document.body.getAttribute('data-app')){
                if (o.app==1){
                    window.location.href = "/";
                }
                if (o.app==2){
                    window.location.href = "/wipe";
                }
            }
            if (o.cnn != document.body.getAttribute('data-confno')){
                window.location.href = window.location.origin;
            }
            setTimeout(updatestatus, 300);
        }
    };
    xmlhttp.send("{\"a\":0}");
}

function updateform(url, data){
    xmlhttp = new XMLHttpRequest();
    xmlhttp.open("POST", url, true);
    xmlhttp.setRequestHeader('Content-Type', 'application/json')
    xmlhttp.onreadystatechange = function () {
        if (xmlhttp.readyState == 4 && xmlhttp.status == 200) {
            try {
                JSON.parse(xmlhttp.responseText);
            }
            catch(ex) {
                return false;
            }
            let o = JSON.parse(xmlhttp.responseText);
            document.getElementById('st').innerHTML = o.status;
            if (o.app==2){
                window.location.href = "/wipe";
            }
            if (o.cnn != confno){
                window.location.href = window.location.origin;
            }
            setTimeout(updatestatus, 300);
        }
    };
    xmlhttp.send(JSON.stringify(data));
}

window.onload = function() {
    setTimeout(updatestatus, 300);
    let i = document.getElementById('ki');
    //modal
    document.addEventListener('click', function (e) {
        console.log(e);
        e = e || window.event;

        var target = e.target || e.srcElement;
        if (target.hasAttribute('data-toggle') && target.getAttribute('data-toggle') == 'modal') {
            if (target.hasAttribute('data-target')) {
                var m_ID = target.getAttribute('data-target');
                document.getElementById(m_ID).classList.add('open');
                e.preventDefault();
            }
        }
        if (target.type == 'button'){
            fe = target.form.elements;
            console.log(fe);
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
            if (target.form.hasAttribute("data-t")){
                url = target.form.getAttribute("data-t");
                console.log(url);
                updateform(url, data);
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
