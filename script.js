document.getElementById('myForm').addEventListener('submit', function(event) {
    event.preventDefault(); // Prevent full page reload
  
    var userInput = document.getElementById('user-input').value;
    var xhr = new XMLHttpRequest();
    xhr.open('POST', '/cgi-bin/mycgi', true);
    xhr.setRequestHeader('Content-Type', 'application/x-www-form-urlencoded');
    xhr.send('user-input=' + encodeURIComponent(userInput));
  
    xhr.onreadystatechange = function() {
      if (xhr.readyState === 4 && xhr.status === 200) {
        var response = xhr.responseText;
        var chatLog = document.getElementById('chat-log');
        var newMessage = document.createElement('div');
        newMessage.innerHTML = response;
        newMessage.classList.add('message'); // Add class for styling if needed
        chatLog.insertBefore(newMessage, chatLog.firstChild); // Prepend new message to chat log
        document.getElementById('user-input').value = '';
      }
    };
  });