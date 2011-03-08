/* $Id: admin_menu.js,v 1.2.2.6 2008/06/08 14:09:11 sun Exp $ */

$(document).ready(function() {
  // Apply margin-top if enabled; directly applying marginTop doesn't work in IE.
  if (Drupal.settings.admin_menu.margin_top) {
    $('body').addClass('admin-menu');
  }

  // Collapse fieldsets on Modules page.
  if (Drupal.settings.admin_menu.tweak_modules) {
    $('[id^="system-modules"] fieldset:not(.collapsed)').addClass('collapsed');
  }

  // Collapse menus on menu administration page.
  if (Drupal.settings.admin_menu.tweak_menu) {
    $('div.box:not(.admin-menu-menu-processed)').each(function() {
      $(this).addClass('admin-menu-menu-processed')
        .find('.content').hide().addClass('collapsible').addClass('collapsed').end()
        .find('h2').css('cursor', 'pointer').click(function() {
          $(this).next('.content').toggleClass('collapsed').slideToggle('fast');
        });
    });
  }

  // Hover emulation for IE 6.
  if ($.browser.msie && parseInt(jQuery.browser.version) == 6) {
    $('#admin-menu li').hover(function() {
      $(this).addClass('iehover');
    }, function() {
      $(this).removeClass('iehover');
    });
  }
  
  // Delayed mouseout.
  $('#admin-menu li').hover(function() {
    // Stop the timer.
    clearTimeout(this.sfTimer);
    // Display child lists.
    $('> ul', this).css('left', 'auto')
      // Immediately hide nephew lists.
      .parent().siblings('li').children('ul').css('left', '-999em');
  }, function() {
    // Start the timer.
    var uls = $('> ul', this);
    this.sfTimer = setTimeout(function() {
      uls.css('left', '-999em');
    }, 400);
  });
});
