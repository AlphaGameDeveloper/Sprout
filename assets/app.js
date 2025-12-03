document.addEventListener('DOMContentLoaded', ()=>{
  const form = document.getElementById('wol-form');
  const hostsList = document.getElementById('hosts');

  function renderHosts(){
    const hosts = JSON.parse(localStorage.getItem('wol_hosts')||'[]');
    hostsList.innerHTML = '';
    hosts.forEach((h, i)=>{
      const li = document.createElement('li');
      li.className = 'host-item';
      li.innerHTML = `<div>${h.name||h.mac} <small style="color:var(--muted);display:block">${h.mac} • ${h.broadcast||''}</small></div>` +
        `<div><button data-i="${i}" class="wake">Wake</button></div>`;
      hostsList.appendChild(li);
    });
  }

  hostsList.addEventListener('click', async (ev)=>{
    if(ev.target.matches('.wake')){
      const i = Number(ev.target.dataset.i);
      const hosts = JSON.parse(localStorage.getItem('wol_hosts')||'[]');
      const host = hosts[i];
      await sendWake(host.mac, host.broadcast);
    }
  });

  async function sendWake(mac, broadcast){
    try{
      const payload = { mac, broadcast };
      await fetch('/api/wake', {method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(payload)});
      alert('Magic packet sent');
    }catch(e){
      alert('Failed to send');
    }
  }

  form.addEventListener('submit', async (ev)=>{
    ev.preventDefault();
    const mac = document.getElementById('mac').value.trim();
    const broadcast = document.getElementById('broadcast').value.trim();

    const hosts = JSON.parse(localStorage.getItem('wol_hosts')||'[]');
    hosts.unshift({mac,broadcast});
    localStorage.setItem('wol_hosts', JSON.stringify(hosts.slice(0,10)));
    renderHosts();

    await sendWake(mac,broadcast);
  });

  renderHosts();
});
