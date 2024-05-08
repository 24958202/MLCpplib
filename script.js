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

          // Append user input to chat log
          var userMessage = document.createElement('div');
          userMessage.innerHTML = `<p class="user-message">${userInput}</p>`; // User's message
          userMessage.classList.add('right-align'); // Add class for right alignment
          chatLog.appendChild(userMessage); // Append new message to chat log

          // Append server response to chat log
          var serverMessage = document.createElement('div');
          serverMessage.innerHTML = response;
          serverMessage.classList.add('message'); // Add class for styling if needed
          chatLog.appendChild(serverMessage); // Append new message to chat log

          document.getElementById('user-input').value = '';
          chatLog.scrollTop = chatLog.scrollHeight; // Scroll to the bottom of the chat log
      }
  };
});